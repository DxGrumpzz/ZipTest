#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>

#include "aes256.hpp"

#include "deflate.h"

#undef _CRT_SECURE_NO_DEPRECATE  
#undef _CRT_NONSTDC_NO_DEPRECATE

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




void ExtractSingleFile(uint8_t* zipFileData, int fileHeaderOffset, ZipEncryption encryptionType)
{
    uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];


    const short filenameLength = (fileHeaderPointer[26] |
                                  fileHeaderPointer[27] << 8);

    const short extraFieldLength = (fileHeaderPointer[28] |
                                    fileHeaderPointer[29] << 8);


    uint8_t* extraField = new uint8_t[extraFieldLength] { 0 };
    memcpy_s(extraField, extraFieldLength, &fileHeaderPointer[static_cast<size_t>(filenameLength) + 30], extraFieldLength);


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
            }
            else if (encryptionType == ZipEncryption::AES)
            {
                uint8_t* encryptedFileDataBuffer = new uint8_t[static_cast<size_t>(compressedSize)] { 0 };

                uint8_t* fileDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];
                uint8_t* fileDataPointerEnd = &fileDataPointer[compressedSize];

                memcpy_s(encryptedFileDataBuffer, compressedSize, fileDataPointer, compressedSize);


                std::vector<unsigned char> key("rwg123", "rwg123" + 7);

                std::vector<unsigned char> decryptedFileData;
                std::vector<unsigned char> encryptedFileData(encryptedFileDataBuffer, encryptedFileDataBuffer + compressedSize);

                Aes256::decrypt(key, encryptedFileData, decryptedFileData);

                decryptedFileData.insert(decryptedFileData.begin(), 0xDA);
                decryptedFileData.insert(decryptedFileData.begin(), 0x78);


                uint8_t* decryptedData = new uint8_t[compressedSize] { 0 };
                uint8_t* uncompressedFileData = new uint8_t[uncompressedSize] { 0 };

                memcpy_s(decryptedData, compressedSize, &decryptedFileData[0], compressedSize);
                uLong uncompressedFileSize = 0;

                int result = uncompress(uncompressedFileData, &uncompressedFileSize, decryptedData, compressedSize + 2);

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

                __debugbreak();
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
            }
            else if (encryptionType == ZipEncryption::AES)
            {
                const char* aesKey = "rwg123";
                const size_t aesKeyLength = strlen(aesKey);

                uint8_t* const fileHeaderDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];
                uint8_t* fileHeaderDataPointerEnd = &fileHeaderDataPointer[uncompressedSize];

                uint8_t aesLength = extraField[8];

                // 128 bit
                if (aesLength == 1)
                {

                }
                // 192 bit
                else if (aesLength == 2)
                {

                }
                // 256 bit
                else if (aesLength == 3)
                {
                    const uint8_t saltSizeInBytes = 16;

                    uint8_t salt[saltSizeInBytes] { 0 };

                    memcpy_s(salt, saltSizeInBytes, fileHeaderDataPointer, saltSizeInBytes);


                    uint16_t passwordVerificationValue = (fileHeaderDataPointer[saltSizeInBytes] |
                                                          fileHeaderDataPointer[saltSizeInBytes + 1]);


                    const size_t fileDataOffset = saltSizeInBytes + 2;


                    uint8_t* encryptedFileData = new uint8_t[uncompressedSize] { 0 };

                    memcpy_s(encryptedFileData, uncompressedSize, &fileHeaderDataPointer[fileDataOffset], uncompressedSize);


                    std::vector<unsigned char> key(aesKey, aesKey + aesKeyLength);
                    std::vector<unsigned char> encryptedData(encryptedFileData, encryptedFileData + uncompressedSize);
                    std::vector<unsigned char> decryptedData;

                    Aes256::decrypt(key, encryptedData, decryptedData);

                    __debugbreak();

                    delete[] encryptedFileData;
                    encryptedFileData = nullptr;
                };


                __debugbreak();
            };


            break;
        };

    };
};


ZipEncryption GetEncryptionType(const std::vector<uint8_t>& centralDirectory)
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


int main()
{
    /*  {
          const char* _key = "rwg123";
          size_t _keyLength = strlen(_key);
          std::vector<unsigned char> key(_key, _key + _keyLength);

          const char* _text = "Thims ims a vermly importamt temxt. pamsta";
          size_t _textLength = strlen(_text);
          std::vector<unsigned char> text(_text, _text + _textLength);

          std::vector<unsigned char> encrypted;

          Aes256::encrypt(key, text, encrypted);

          std::vector<unsigned char> decrypted;

          Aes256::decrypt(key, encrypted, decrypted);

          __debugbreak();
      };*/

    const bool loadLockedZip = true;

    const wchar_t* zipFilepath;

    if (loadLockedZip == true)
        zipFilepath = L"ZipTest AES.zip";
    // zipFilepath = L"ZipTest ZipCrypto.zip";
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


    const std::vector<uint8_t> centralDirectory = centralDirectories[3];



    const int fileHeaderOffset = (centralDirectory[42] |
                                  centralDirectory[43] << 8 |
                                  centralDirectory[44] << 16 |
                                  centralDirectory[45] << 24);

    ZipEncryption encryptionType = GetEncryptionType(centralDirectory);

    ExtractSingleFile(zipFileBuffer, fileHeaderOffset, encryptionType);


    __debugbreak();


    delete[] zipFileBuffer;
    zipFileBuffer = nullptr;
};