#pragma once

#include "binary_reader.h"
#include "three_node.h"

#include <vector>
#include <map>
#include <list>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <memory>

class invalid_input_file : public std::exception {
public:
	virtual char const* what() const {
		return "invalid_input_file :: Unable to open input file.";
	}
};

class invalid_data : public std::exception {
public:
	virtual char const* what() const {
		return "invalid_data :: Unable to compress data.";
	}
};

namespace std {
	auto begin(std::ifstream& stream)
	{
		return std::istream_iterator<char>(stream);
	}

	auto end(std::ifstream& stream)
	{
		return std::istream_iterator<char>();
	}
}

template<class Iterable>
auto collect_statistic(Iterable& input)
{
	auto begin = std::begin(input);
	auto end = std::end(input);
	typedef std::iterator_traits<decltype(begin)>::value_type value_type;
	typedef std::map<value_type, int> statistic_type;
	statistic_type statistic;
	std::for_each(begin, end, [&statistic](value_type value) { statistic[value]++; });
	return statistic;
}

class static_huffman_policy {

	typedef std::pair<char, int> node_value_type;
	typedef three_node<node_value_type, adder_union_policy> three_node_type;
	typedef std::vector<bool> huffman_code_type;
	typedef std::map<char, huffman_code_type> huffman_codes_storage;
	typedef std::map<char, int> statistic_type;

	static void sort_nodes(std::list<std::shared_ptr<three_node_type>> &nodes)
	{
		nodes.sort([](const std::shared_ptr<three_node_type>& pFirst, const std::shared_ptr<three_node_type>& pSecond) { return pFirst->m_Value.second < pSecond->m_Value.second; });
	}

public:

	static void compress(const std::string& sInput, const std::string& sOutput)
	{
		std::ifstream input(sInput);

		if (!input.is_open())
		{
			throw invalid_input_file();
		}

		auto statistic = collect_statistic(input);
		typedef decltype(statistic)::value_type statistic_value_type;

		std::list<std::shared_ptr<three_node_type>>nodes;
		std::transform(statistic.begin(), statistic.end(), std::back_inserter(nodes), [](const statistic_value_type& value) {
			return std::make_shared<three_node_type>(value);
		});

		if (nodes.empty())
		{
			throw invalid_data();
		}

		std::list<std::shared_ptr<three_node_type>>nodes_copy{ nodes };

		sort_nodes(nodes);
		create_huffman_tree(nodes);

		auto huffman_codes = get_huffman_codes(nodes_copy);

		compress_internal(statistic, huffman_codes, sInput, sOutput);
	}

	static void decompress(const std::string& sInput, const std::string& sOutput)
	{
		std::ifstream input(sInput);

		if (!input.is_open())
		{
			throw invalid_input_file();
		}

		auto statistic = read_header(input);
		typedef decltype(statistic)::value_type statistic_value_type;

		std::list<std::shared_ptr<three_node_type>>nodes;
		std::transform(statistic.begin(), statistic.end(), std::back_inserter(nodes), [](const statistic_value_type& value) {
			return std::make_shared<three_node_type>(value);
		});

		if (nodes.empty())
		{
			throw invalid_data();
		}

		sort_nodes(nodes);
		create_huffman_tree(nodes);

		decompress_internal(statistic, nodes.front(), sInput, sOutput);
	}

private:

	static void create_huffman_tree(std::list<std::shared_ptr<three_node_type>> &nodes)
	{
		while (nodes.size() != 1)
		{
			auto pLeft = nodes.front();
			nodes.pop_front();
			auto pRigth = nodes.front();
			nodes.pop_front();
			auto new_node = std::make_shared<three_node_type>(pRigth, pLeft);
			pLeft->set_parent(new_node);
			pRigth->set_parent(new_node);
			nodes.push_front(new_node);
			sort_nodes(nodes);
		}
	}
	static huffman_codes_storage get_huffman_codes(const std::list<std::shared_ptr<three_node_type>> &nodes_copy)
	{
		huffman_codes_storage storage;
		for (const auto &node : nodes_copy)
		{
			huffman_codes_storage::key_type symbol = node->m_Value.first;
			std::shared_ptr<three_node_type> current_node = node;
			while (!current_node->is_root())
			{
				if (current_node == current_node->m_pParent->m_pLeft)
				{
					storage[symbol].push_back(0);
				}
				else if (current_node == current_node->m_pParent->m_pRigth)
				{
					storage[symbol].push_back(1);
				}

				current_node = current_node->m_pParent;
			}

			std::reverse(storage[symbol].begin(), storage[symbol].end());
		}
		return storage;
	}

	static void compress_internal(const statistic_type& statistic, const huffman_codes_storage& huffman_codes, const std::string& sInput, const std::string& sOutput)
	{
		std::ifstream input(sInput);
		std::ofstream output(sOutput, std::ios::binary);

		huffman_codes_storage::key_type symbol, buffer{ 0 };
		static const unsigned int byte_size = 8;
		unsigned int bit_count{ 0 };

		make_header(statistic, output);

		while (input.get(symbol))
		{
			auto iter = huffman_codes.find(symbol);
			if (iter != huffman_codes.end())
			{
				const huffman_code_type& code = iter->second;
				for (auto bit : code)
				{
					buffer = buffer | (bit << (7 - bit_count));
					bit_count++;
					if (bit_count == byte_size)
					{
						bit_count = 0;
						output.put(buffer);
						buffer = 0;
					}
				}
			}
		}

		if (buffer)
		{
			output.put(buffer);
		}

		input.close();
		output.close();
	}
	static void decompress_internal(const statistic_type& statistic, const std::shared_ptr<three_node_type>& tree_root, const std::string& sInput, const std::string& sOutput)
	{
		std::ifstream input(sInput, std::ios::binary);
		std::ofstream output(sOutput);

		unsigned long file_size{ 0 };
		std::for_each(statistic.begin(), statistic.end(), [&file_size](const statistic_type::value_type& value) { file_size += value.second; });

		const unsigned int header_size = sizeof(std::size_t) + statistic.size() + statistic.size() * sizeof(std::size_t);
		input.seekg(header_size);

		statistic_type::key_type symbol;
		static const unsigned int byte_size = 8;

		unsigned int bit_count{ 0 };
		unsigned long byte_count{ 0 };

		auto root_copy = tree_root;

		if (root_copy->is_leaf())
		{
			while (byte_count != file_size)
			{
				output.put(root_copy->m_Value.first);
				byte_count++;
			}
		}
		else
		{
			while (input.get(symbol))
			{
				bit_count = 0;

				while (bit_count != byte_size && byte_count != file_size)
				{
					bool bIsRigth = (symbol) & (1 << (7 - bit_count));

					if (!bIsRigth)
					{
						root_copy = root_copy->m_pLeft;
					}
					else
					{
						root_copy = root_copy->m_pRigth;
					}

					if (root_copy->is_leaf())
					{
						output.put(root_copy->m_Value.first);
						root_copy = tree_root;
						byte_count++;
					}

					bit_count++;
				}
			}
		}

		input.close();
		output.close();
	}

	static void make_header(const statistic_type& statistic, std::ofstream& output)
	{
		static const std::string& header_name = "SHF";
		binary_writer<std::string>::write(output, header_name);
		binary_writer<std::size_t>::write(output, statistic.size());
		binary_writer<statistic_type>::write(output, statistic);
	}

	static statistic_type read_header(std::ifstream& input)
	{
		static const std::string expected_header_name = "SHF";

		statistic_type storage;
		std::size_t symbols_size;
		std::string header_name;

		binary_reader<std::string>::read(input, header_name, expected_header_name.size());

		if (header_name != expected_header_name)
		{
			throw invalid_data();
		}

		binary_reader<std::size_t>::read(input, symbols_size);
		binary_reader<statistic_type>::read(input, storage, symbols_size);

		return storage;
	}
};
