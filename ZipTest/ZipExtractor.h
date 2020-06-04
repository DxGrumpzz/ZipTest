#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

#include "deflate.h"


namespace ZipExtractor
{
    constexpr int PK_FILE_HEADER_SIGNATURE = 0x504b0304;

    constexpr int PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN = 0x2014B50;

    constexpr int PK_END_OF_CENTRAL_DIRECTORY = 0x504b0506;


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


    enum class ZipEncryption
    {
        None = 0,
        AES = 1,
        ZipCrypto = 2,
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




    void ExtractSingleFile(uint8_t* zipFileData, int fileHeaderOffset, ZipExtractor::ZipEncryption encryptionType, std::string outputFolder)
    {
        uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];


        const short filenameLength = (fileHeaderPointer[26] |
                                      fileHeaderPointer[27] << 8);

        const short extraFieldLength = (fileHeaderPointer[28] |
                                        fileHeaderPointer[29] << 8);


        uint8_t* extraField = nullptr;
        if (extraFieldLength != 0)
        {
            extraField = new uint8_t[extraFieldLength] { 0 };
            memcpy_s(extraField, extraFieldLength, &fileHeaderPointer[static_cast<size_t>(filenameLength) + 30], extraFieldLength);
        };


        short compressionMethod = 0;
        if (encryptionType == ZipEncryption::None)
        {
            compressionMethod = (fileHeaderPointer[8] |
                                 fileHeaderPointer[9] << 8);
        }
        else if (encryptionType == ZipEncryption::AES)
        {
            compressionMethod = (extraField[static_cast<short>(9)] |
                                 extraField[static_cast<short>(10)] << 8);
        };

        const size_t compressedSize = (fileHeaderPointer[18] |
                                       fileHeaderPointer[19] << 8 |
                                       fileHeaderPointer[20] << 16 |
                                       fileHeaderPointer[21] << 24);


        const int uncompressedSize = (fileHeaderPointer[22] |
                                      fileHeaderPointer[23] << 8 |
                                      fileHeaderPointer[24] << 16 |
                                      fileHeaderPointer[25] << 24);


        const uint16_t generalPurposeFlag = (fileHeaderPointer[6] |
                                             fileHeaderPointer[7] << 8);



        switch ((CompressionMethod)compressionMethod)
        {
            case CompressionMethod::Deflated:
            {

                if (encryptionType == ZipEncryption::None)
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



                    std::string outputString;
                    outputString.append(outputFolder);
                    outputString.append("/");
                    outputString.append(filename);


                    std::ofstream output(outputString, std::ios::binary);

                    output.write(reinterpret_cast<char*>(&uncompressedFileData[0]), uncompressedFileSize);
                    output.close();


                    delete[] filename;
                    filename = nullptr;

                    delete[] uncompressedFileData;
                    uncompressedFileData = nullptr;

                    delete[] fileDataBuffer;
                    fileDataBuffer = nullptr;
                }
                else if (encryptionType == ZipEncryption::AES)
                {
                    throw std::exception("AES encryption isn't supported, yet.");
                };

                break;
            };


            case CompressionMethod::None:
            {
                if (encryptionType == ZipEncryption::None)
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
                        std::filesystem::path filepath(outputFolder);
                        filepath.append(filename);
                        filepath.remove_filename();
                        filepath.make_preferred();

                        std::filesystem::create_directories(filepath);
                    };


                    std::string outputString;
                    outputString.append(outputFolder);
                    outputString.append("/");
                    outputString.append(filename);


                    std::ofstream output(outputString, std::ios::binary);

                    output.write(reinterpret_cast<char*>(&fileHeaderDataPointer[0]), uncompressedSize);
                    output.close();

                    delete[] filename;
                    filename = nullptr;
                }
                else if (encryptionType == ZipEncryption::AES)
                {
                    throw std::exception("AES encryption isn't supported, yet.");
                };


                break;
            };

        };


        if (extraField != nullptr)
        {
            delete[] extraField;
            extraField = nullptr;
        };

    };


    void ExtractSingleFolder(uint8_t* zipFileData, int fileHeaderOffset, ZipEncryption encryptionType, const std::string& outputFolder)
    {
        uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];


        const short folderNameLength = (fileHeaderPointer[26] |
                                        fileHeaderPointer[27] << 8);

        const char* folderNamePointer = reinterpret_cast<char*>(&fileHeaderPointer[30]);

        std::string filename(outputFolder);
        filename.append("/");
        filename.append(folderNamePointer, folderNameLength);

        std::filesystem::create_directories(filename);
    };



    ZipExtractor::ZipEncryption GetEncryptionType(const std::vector<uint8_t>& centralDirectory)
    {
        const unsigned short generalPurposeBitFlag = (centralDirectory[8] |
                                                      centralDirectory[9] << 8);

        bool isEncrypted = generalPurposeBitFlag & 1 << 0;


        const unsigned short compressionMethod = (centralDirectory[10] |
                                                  centralDirectory[11] << 8);


        const unsigned int crc32 = (centralDirectory[16] |
                                    centralDirectory[17]);

        if ((isEncrypted == true) &&
            (compressionMethod == 99) &&
            (crc32 == 0))
        {
            return ZipEncryption::AES;
        }
        else
        {
            return ZipEncryption::None;
        };
    };


    void ReadZipFile(std::string zipFilepath, uint8_t*& zipFileBuffer, size_t& zipFileBufferLength)
    {

        std::ifstream fileStream(zipFilepath, std::ios::binary);


        if (fileStream.is_open() == false)
        {
            throw std::exception("Error opening file");
        };

        if (fileStream.good() == false)
        {
            throw std::exception("File error");
        };


        zipFileBufferLength = std::filesystem::file_size(zipFilepath);

        zipFileBuffer = new uint8_t[zipFileBufferLength] { 0 };
        fileStream.read((char*)&zipFileBuffer[0], zipFileBufferLength);
    };



};