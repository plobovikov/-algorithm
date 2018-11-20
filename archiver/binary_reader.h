#pragma once

#include <string>
#include <map>

template<class T, class Enable = void>
struct binary_reader;

template<class T>
struct binary_reader<T, std::enable_if_t<std::is_fundamental<T>::value > > {
	template<class Stream>
	static void read(Stream& stream, T& value) {
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
	}
};

template<>
struct binary_reader<std::string, void> {
	template<class Stream>
	static void read(Stream& stream, std::string& value, std::size_t size) {
		value.resize(size);
		for (auto & symbol : value)
		{
			stream.get(symbol);
		}
	}
};

template<class K, class V>
struct binary_reader<std::map<K, V>, void> {
	template<class Stream>
	static void read(Stream& stream, std::map<K, V>& value, std::size_t size) {
		for (std::size_t i = 0; i < size; i++)
		{
			K key;
			binary_reader<K>::read(stream, key);
			binary_reader<V>::read(stream, value[key]);
		}
	}
};

template<class T, class Enable = void >
struct binary_writer;

template<class T>
struct binary_writer<T, std::enable_if_t<std::is_fundamental<T>::value > > {
	template<class Stream>
	static void write(Stream& stream, T value) {
		stream.write(reinterpret_cast<char*>(&value), sizeof(T));
	}
};

template<>
struct binary_writer<std::string, void> {
	template<class Stream>
	static void write(Stream& stream, const std::string& value) {
		stream << value;
	}
};

template<class K, class V>
struct binary_writer<std::map<K, V>, void> {
	template<class Stream>
	static void write(Stream& stream, const std::map<K, V>& value) {
		for (auto elem : value)
		{
			binary_writer<K>::write(stream, elem.first);
			binary_writer<V>::write(stream, elem.second);
		}
	}
};