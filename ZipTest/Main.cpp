#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>

#include "deflate.h"



constexpr int PK_FILE_HEADER_SIGNATURE = 0x504b0304;

constexpr int PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN = 0x2014B50;

constexpr int PK_END_OF_CENTRAL_DIRECTORY = 0x504b0506;

const char* zipOutFilepath = "ZipTest out";


enum class CompressionMethod : short
{
    None = 0,

    Shrunk = 1,

    LZW = 1,

    ReducedWithCompressionFactor1 = 2,
    ReducedWithCompressionFactor2 = 3,
    ReducedWithCompressionFactor3 = 4,
    ReducedWithCompressionFactor4 = 5,

    Imploded = 6,

    Reserved_1 = 7,

    Deflated = 8,

    EnhancedDeflated = 9,

    PKWareDCLimploded = 10,

    Reserved_2 = 11,

    BZIP2 = 12,

    Reserved_3 = 13,

    LZMA = 14,

    Reserved_4 = 15,
    Reserved_5 = 16,
    Reserved_6 = 17,

    IBM_TERSE = 18,
    IBM_LZ77z = 19,

    PPMd_Version_I_Rev_1 = 98,
};



void GetEndCentralDirectory(const uint8_t* zipFileData, const uintmax_t& zipFileDataLength, std::vector<uint8_t>& outdata)
{
    uintmax_t indexer = zipFileDataLength;
    uintmax_t dataIndexer = indexer - 4;

    int pkSignature = 0;

    while (indexer > 0)
    {
        pkSignature = zipFileData[dataIndexer];
        pkSignature <<= 8;
        pkSignature |= zipFileData[dataIndexer + 1];
        pkSignature <<= 8;
        pkSignature |= zipFileData[dataIndexer + 2];
        pkSignature <<= 8;
        pkSignature |= zipFileData[dataIndexer + 3];

        outdata.insert(outdata.begin(), zipFileData[indexer - 1]);

        if (pkSignature == PK_END_OF_CENTRAL_DIRECTORY)
        {
            outdata.insert(outdata.begin(), zipFileData[indexer - 2]);
            outdata.insert(outdata.begin(), zipFileData[indexer - 3]);
            outdata.insert(outdata.begin(), zipFileData[indexer - 4]);

            return;
        };

        dataIndexer--;
        indexer--;
    };
};


void GetCentralDirectories(uint8_t* zipFileData, const uintmax_t& zipFileDataLength, const int& centralDirectoryOffset, std::vector<std::vector<uint8_t>>& centralDirectoriesOut)
{
    uint8_t* centralDirectoryPointer = &zipFileData[centralDirectoryOffset];
    uint8_t const* const centralDirectoryPointerEnd = &zipFileData[zipFileDataLength];

    int centralDirectoySignature = 0;

    while (centralDirectoryPointer != centralDirectoryPointerEnd)
    {
        centralDirectoySignature = (*centralDirectoryPointer |
                                    *(centralDirectoryPointer + 1) << 8 |
                                    *(centralDirectoryPointer + 2) << 16 |
                                    *(centralDirectoryPointer + 3) << 24);


        if (centralDirectoySignature == PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN)
        {
            centralDirectoriesOut.emplace_back(std::vector<uint8_t>());
        };

        (*(centralDirectoriesOut.end() - 1)).emplace_back(*centralDirectoryPointer);
        centralDirectoryPointer++;
    };

};



void ExtractSingleFile(uint8_t* zipFileData, const int& fileHeaderOffset)
{
    uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];


    const short filenameLength = (fileHeaderPointer[26] |
                                  fileHeaderPointer[27] << 8);

    const short extraFieldLength = (fileHeaderPointer[28] |
                                    fileHeaderPointer[29] << 8);



    const short compressionMethod = (fileHeaderPointer[8] |
                                     fileHeaderPointer[9] << 8);


    const int compressedSize = (fileHeaderPointer[18] |
                                fileHeaderPointer[19] << 8 |
                                fileHeaderPointer[20] << 16 |
                                fileHeaderPointer[21] << 24);


    const int uncompressedSize = (fileHeaderPointer[22] |
                                  fileHeaderPointer[23] << 8 |
                                  fileHeaderPointer[24] << 16 |
                                  fileHeaderPointer[25] << 24);



    switch ((CompressionMethod)compressionMethod)
    {

        case CompressionMethod::Deflated:
        {
            uint8_t* fileHeaderDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];

            uint8_t* fileDataBuffer = new uint8_t[static_cast<size_t>(uncompressedSize) + 2] { 0 };
            fileDataBuffer[0] = 0x78;
            fileDataBuffer[1] = 0xDA;

            memcpy_s(fileDataBuffer + 2, uncompressedSize, fileHeaderDataPointer, uncompressedSize);

            uint8_t* uncompressedFileData = new uint8_t[uncompressedSize] { 0 };
            uLong uncompressedFileSize = static_cast<uLong>(uncompressedSize);


            int result = uncompress(uncompressedFileData, &uncompressedFileSize, fileDataBuffer, compressedSize + 2);

            char* filename = new char[static_cast<size_t>(filenameLength) + 1] { 0 };
            memcpy_s(filename, static_cast<size_t>(filenameLength) + 1, reinterpret_cast<char*>(&fileHeaderPointer[30]), filenameLength);


            size_t slashCount = 0;

            char* fileNamePointer = filename;
            while (*fileNamePointer != '\0')
            {
                if (*fileNamePointer == '/')
                {
                    slashCount++;
                };

                fileNamePointer++;
            };


            if (slashCount > 0)
            {
                std::filesystem::path filepath(zipOutFilepath);
                filepath.append(filename);
                filepath.remove_filename();
                filepath.make_preferred();

                std::filesystem::create_directories(filepath);
            };

            std::string outputString;
            outputString.append(zipOutFilepath);
            outputString.append("/");
            outputString.append(filename);



            std::ofstream output(outputString, std::ios::binary);

            output.write(reinterpret_cast<char*>(&uncompressedFileData[0]), uncompressedFileSize);
            output.close();


            __debugbreak();


            delete[] filename;
            filename = nullptr;

            delete[] uncompressedFileData;
            uncompressedFileData = nullptr;

            delete[] fileDataBuffer;
            fileDataBuffer = nullptr;

            break;
        };


        case CompressionMethod::None:
        {
            uint8_t* fileHeaderDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];

            char* filename = new char[static_cast<size_t>(filenameLength) + 1] { 0 };

            memcpy_s(filename, static_cast<size_t>(filenameLength) + 1, reinterpret_cast<char*>(&fileHeaderPointer[30]), filenameLength);


            size_t slashCount = 0;

            char* fileNamePointer = filename;
            while (*fileNamePointer != '\0')
            {
                if (*fileNamePointer == '/')
                {
                    slashCount++;
                };

                fileNamePointer++;
            };


            if (slashCount > 0)
            {
                std::filesystem::path filepath(zipOutFilepath);
                filepath.append(filename);
                filepath.remove_filename();
                filepath.make_preferred();

                std::filesystem::create_directories(filepath);
            };


            std::string outputString;
            outputString.append(zipOutFilepath);
            outputString.append("/");
            outputString.append(filename);


            std::ofstream output(outputString, std::ios::binary);

            output.write(reinterpret_cast<char*>(&fileHeaderDataPointer[0]), uncompressedSize);
            output.close();

            __debugbreak();

            delete[] filename;
            filename = nullptr;

            break;
        };

    };
};



int main()
{
    const bool loadLockedZip = false;

    const wchar_t* zipFilepath;

    if (loadLockedZip == true)
        zipFilepath = L"Locked ZipTest.zip";
    else
        zipFilepath = L"ZipTest.zip";



    std::ifstream fileStream(zipFilepath, std::ios::binary);


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

    uint8_t* zipFileBuffer = new uint8_t[zipFileSize] { 0 };
    fileStream.read((char*)&zipFileBuffer[0], zipFileSize);

    std::vector<std::vector<uint8_t>> files;

    int pkSignature = 0;

    uintmax_t indexer = 0;

    int fileIndexer = -1;

    while (indexer < zipFileSize)
    {

        pkSignature = zipFileBuffer[indexer];
        pkSignature <<= 8;

        // Ignoring this warning because apparently it is a bug in Visual Studio's static code analyzer
        #pragma warning(suppress : 6385)
        pkSignature |= zipFileBuffer[indexer + 1];

        pkSignature <<= 8;
        pkSignature |= zipFileBuffer[indexer + 2];
        pkSignature <<= 8;
        pkSignature |= zipFileBuffer[indexer + 3];


        if (pkSignature == PK_FILE_HEADER_SIGNATURE)
        {
            files.push_back(std::vector<uint8_t>());
            fileIndexer++;
        };

        files[fileIndexer].emplace_back(zipFileBuffer[indexer]);

        indexer++;
    };


    std::vector<uint8_t> endCentralDirectoryOut;
    GetEndCentralDirectory(zipFileBuffer, zipFileSize, endCentralDirectoryOut);


    int centralDirectoryOffset = (endCentralDirectoryOut[16] |
                                  endCentralDirectoryOut[17] << 8 |
                                  endCentralDirectoryOut[18] << 16 |
                                  endCentralDirectoryOut[19] << 24);

    std::vector<std::vector<uint8_t>> centralDirectories;
    GetCentralDirectories(zipFileBuffer, zipFileSize, centralDirectoryOffset, centralDirectories);




    const std::vector<uint8_t> centralDirectory = centralDirectories[1];


    const int fileHeaderOffset = (centralDirectory[42] |
                                  centralDirectory[43] << 8 |
                                  centralDirectory[44] << 16 |
                                  centralDirectory[45] << 24);


    ExtractSingleFile(zipFileBuffer, fileHeaderOffset);


    __debugbreak();


    delete[] zipFileBuffer;
    zipFileBuffer = nullptr;
};