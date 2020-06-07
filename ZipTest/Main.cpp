#include <string>
#include <vector>

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



    std::vector<uint8_t> zipFileBuffer;
    zExtr::ReadZipFile(zipFilepath, zipFileBuffer);


    std::vector<uint8_t> endCentralDirectory;
    zExtr::GetEndCentralDirectory(zipFileBuffer, endCentralDirectory);


    std::vector<std::vector<uint8_t>> centralDirectories;
    zExtr::GetCentralDirectories(zipFileBuffer, endCentralDirectory, centralDirectories);



    for (const std::vector<uint8_t>& centralDirectory : centralDirectories)
    {
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
            //ExtractSingleFolder(zipFileBuffer, fileHeaderOffset, encryptionType, zipOutFolder);
            ExtractSingleFolder(zipFileBuffer, centralDirectory, encryptionType, zipOutFolder);
        else
            ExtractSingleFile(zipFileBuffer, centralDirectory, encryptionType, zipOutFolder);
    };


};