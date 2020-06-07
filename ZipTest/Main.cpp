#include <string>
#include <vector>

#include "ZipExtractor.h"

#undef _CRT_SECURE_NO_DEPRECATE  
#undef _CRT_NONSTDC_NO_DEPRECATE



int main()
{
    const bool loadLockedZip = false;

    const std::string zipOutFolder("ZipTest out");


    std::string zipFilepath("Test zip files");

    if (loadLockedZip == true)
        zipFilepath.append("/ZipTest AES.zip");
    else
        zipFilepath.append("/ZipTest.zip");



    std::vector<uint8_t> zipFileBuffer;
    ZipExtractor::ReadZipFile(zipFilepath, zipFileBuffer);


    std::vector<uint8_t> endCentralDirectory;
    ZipExtractor::GetEndCentralDirectory(zipFileBuffer, endCentralDirectory);


    std::vector<std::vector<uint8_t>> centralDirectories;
    ZipExtractor::GetCentralDirectories(zipFileBuffer, endCentralDirectory, centralDirectories);



    ZipExtractor::ExtractZip(zipOutFolder, zipFileBuffer, centralDirectories);

};