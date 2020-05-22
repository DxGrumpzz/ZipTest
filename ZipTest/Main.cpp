#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

typedef unsigned char Byte;

constexpr int PK_SIGNATURE = 0x504b0304;

int main()
{
    const wchar_t* zipFilepath = L"TestZip1.zip";

    std::ifstream fileStream(zipFilepath);

    if (fileStream.is_open() == false)
    {
        throw std::exception("Error opening file");
        return 0;
    };

    if (fileStream.good() == false)
    {
        throw std::exception("File error");
        return 0;
    };

    uintmax_t zipFileSize = std::filesystem::file_size(zipFilepath);

    char* buffer = new char[zipFileSize];
    memset(buffer, 0, zipFileSize);

    fileStream.read(buffer, zipFileSize);


    std::vector<std::vector<Byte>> files;

    int pkSignature = 0;

    int indexer = 0;

    int fileIndexer = -1;

    while (indexer < zipFileSize)
    {
        pkSignature = buffer[indexer + 0];
        pkSignature <<= 8;
        pkSignature |= buffer[indexer + 1];
        pkSignature <<= 8;
        pkSignature |= buffer[indexer + 2];
        pkSignature <<= 8;
        pkSignature |= buffer[indexer + 3];

        if (pkSignature == PK_SIGNATURE)
        {
            files.push_back(std::vector<Byte>());
            fileIndexer++;
        };
        
        files[fileIndexer].emplace_back(buffer[indexer]);

        indexer++;
    };


    delete[] buffer;
    buffer = nullptr;
};