#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>


#include "deflate.h"
#include "ZipExtractor.h"

#undef _CRT_SECURE_NO_DEPRECATE  
#undef _CRT_NONSTDC_NO_DEPRECATE


int main()
{
    namespace zExtr = ZipExtractor;

    const bool loadLockedZip = false;

    const std::string zipOutFolder("ZipTest out");


    std::string zipFilepath("Test zip files");

    if (loadLockedZip == true)
        zipFilepath.append("/ZipTest AES.zip");
    else
        zipFilepath.append("/ZipTest.zip");
    


    uint8_t* zipFileBuffer = nullptr;
    size_t zipFileBufferLength = 0;

    zExtr::ReadZipFile(zipFilepath, zipFileBuffer, zipFileBufferLength);


    std::vector<std::vector<uint8_t>> files;

    int pkSignature = 0;

    uintmax_t indexer = 0;

    int fileIndexer = -1;

    while (indexer < zipFileBufferLength)
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


        if (pkSignature == zExtr::PK_FILE_HEADER_SIGNATURE)
        {
            files.push_back(std::vector<uint8_t>());
            fileIndexer++;
        };

        files[fileIndexer].emplace_back(zipFileBuffer[indexer]);

        indexer++;
    };


    std::vector<uint8_t> endCentralDirectoryOut;
    zExtr::GetEndCentralDirectory(zipFileBuffer, zipFileBufferLength, endCentralDirectoryOut);


    int centralDirectoryOffset = (endCentralDirectoryOut[16] |
                                  endCentralDirectoryOut[17] << 8 |
                                  endCentralDirectoryOut[18] << 16 |
                                  endCentralDirectoryOut[19] << 24);

    std::vector<std::vector<uint8_t>> centralDirectories;
    zExtr::GetCentralDirectories(zipFileBuffer, zipFileBufferLength, centralDirectoryOffset, centralDirectories);


    for (const std::vector<uint8_t>& centralDirectory : centralDirectories)
    {
        const int fileHeaderOffset = (centralDirectory[42] |
                                      centralDirectory[43] << 8 |
                                      centralDirectory[44] << 16 |
                                      centralDirectory[45] << 24);

        const unsigned int compressedSize = (centralDirectory[20] |
                                             centralDirectory[21] << 8 |
                                             centralDirectory[22] << 16 |
                                             centralDirectory[23] << 24);

        const unsigned int uncompressedSize = (centralDirectory[24] |
                                               centralDirectory[25] << 8 |
                                               centralDirectory[26] << 16 |
                                               centralDirectory[27] << 24);


        const bool isFolder = (compressedSize == 0 && uncompressedSize == 0) ? true : false;


        zExtr::ZipEncryption encryptionType = zExtr::GetEncryptionType(centralDirectory);

        if (encryptionType == zExtr::ZipEncryption::AES)
            throw std::exception("AES encryption isn't supported, yet.");



        if (isFolder == true)
            ExtractSingleFolder(zipFileBuffer, fileHeaderOffset, encryptionType, zipOutFolder);
        else
            ExtractSingleFile(zipFileBuffer, fileHeaderOffset, encryptionType, zipOutFolder);

    };


    delete[] zipFileBuffer;
    zipFileBuffer = nullptr;
};