#include <string>
#include <vector>

#include "ZipExtractor.h"

#undef _CRT_SECURE_NO_DEPRECATE  
#undef _CRT_NONSTDC_NO_DEPRECATE



int main()
{
    const std::string zipOutFolder("ZipTest out");

    std::string zipFilepath("Test zip files/ZipTest.zip");

    // Read zip file contents
    std::vector<uint8_t> zipFileBuffer;
    ZipExtractor::ReadZipFile(zipFilepath, zipFileBuffer);

    // Get end central directory
    std::vector<uint8_t> endCentralDirectory;
    ZipExtractor::GetEndCentralDirectory(zipFileBuffer, endCentralDirectory);

    // Get central directories
    std::vector<std::vector<uint8_t>> centralDirectories;
    ZipExtractor::GetCentralDirectories(zipFileBuffer, endCentralDirectory, centralDirectories);


    // Extract zip file
    ZipExtractor::ExtractZip(zipOutFolder, zipFileBuffer, centralDirectories);

};