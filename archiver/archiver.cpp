// Лаба4_СД.cpp: определяет точку входа для консольного приложения.
//

#include <iostream>

#include "static_huffman.h"

template<typename CompressionPolicy>
class archiver {
public:
	static void compress(const std::string& sInput, const std::string& sOutput)
	{
		CompressionPolicy::compress(sInput, sOutput);
	}
	static void decompress(const std::string& sInput, const std::string& sOutput)
	{
		CompressionPolicy::decompress(sInput, sOutput);
	}
};

int main()
{
	try {
		archiver<static_huffman_policy>::compress("original.txt", "compressed.txt");
	}
	catch (const invalid_input_file& ex) {
		std::cout << ex.what();
	}
	catch (const invalid_data& ex) {
		std::cout << ex.what();

	}

	try {
		archiver<static_huffman_policy>::decompress("compressed.txt", "decompressed.txt");
	}
	catch (const invalid_input_file& ex) {
		std::cout << ex.what();
	}
	catch (const invalid_data& ex) {
		std::cout << ex.what();

	}

	return 0;
}