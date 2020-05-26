#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>


typedef uint8_t Byte;

constexpr int PK_SIGNATURE = 0x504b0304;

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

const wchar_t* GetLocalZipFileName(std::vector<Byte>& const zipFileData)
{
    const short filenameLength = zipFileData[26];

    wchar_t* filename = new wchar_t[filenameLength + 1] { 0 };

    for (int a = 0; a < filenameLength; a++)
    {
        filename[a] = zipFileData[30 + a];
    };

    return filename;
};


void GetLocalZipFileData(const std::vector<Byte>& zipFileData, std::vector<Byte>& outdata)
{
    const short filenameLength = zipFileData[26];

    const size_t startOfDataIndex = filenameLength + 30;

    for (size_t a = startOfDataIndex; a < zipFileData.size(); a++)
    {
        Byte dataPointer = zipFileData[a];
        outdata.emplace_back(dataPointer);
    };
};


void GetLocalZipFileDataW2(const std::vector<Byte>& zipFileData, wchar_t*& outData, int& outdataLength)
{
    const short filenameLength = zipFileData[26];

    const size_t startOfDataIndex = filenameLength + 30;

    outdataLength = zipFileData.size() - startOfDataIndex;

    outData = new wchar_t[outdataLength + 1] { 0 };

    for (size_t a = startOfDataIndex; a < zipFileData.size(); a++)
    {
        Byte dataPointer = zipFileData[a];
        outData[a - startOfDataIndex] = dataPointer;
    };

    int s = 0;
};



int main()
{
    const wchar_t* zipFilepath = L"TestZip1.zip";

    std::wifstream fileStream(zipFilepath);

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

    wchar_t* buffer = new wchar_t[zipFileSize + 0] { 0 };

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


    Byte comp1 = files[3][8];
    Byte comp2 = files[3][9];

    short compressionMethod = comp1;
    if (comp2 != 0)
    {
        compressionMethod >>= 8;
        compressionMethod |= comp2;
    };


    switch (compressionMethod)
    {
        case (short)CompressionMethod::Deflated:
        {

        };
    };


    for (int a = 0; a < files.size(); a++)
    {
        wchar_t* fileData = nullptr;
        int datalength = 0;

        GetLocalZipFileDataW2(files[a], fileData, datalength);

        const wchar_t* filename = GetLocalZipFileName(files[a]);

        std::wofstream s(filename, std::ios::binary);

        s.write(fileData, datalength);
        s.close();


        delete filename;
        filename = nullptr;
    };


    delete[] buffer;
    buffer = nullptr;
};