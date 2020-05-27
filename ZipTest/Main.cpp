#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>

typedef uint8_t Byte;

constexpr int PK_FILE_HEADER_SIGNATURE = 0x504b0304;

constexpr int PK_CENTRAL_DIRECTORY_SIGNATURE = 0x504b0102;

// Little endian version of the central directory signature
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


void GetLocalZipFileData3(const std::vector<Byte>& zipFileData, Byte*& outData, int& outdataLength)
{
    const short filenameLength = zipFileData[26];

    const size_t startOfDataIndex = filenameLength + 30;

    outdataLength = zipFileData.size() - startOfDataIndex;

    outData = new Byte[outdataLength + 1] { 0 };

    for (size_t a = startOfDataIndex; a < zipFileData.size(); a++)
    {
        Byte dataPointer = zipFileData[a];
        outData[a - startOfDataIndex] = dataPointer;
    };
};



void GetCentralDirectory(const uint8_t* zipFileData, const int& zipFileDataLength, std::vector<Byte>& outdata)
{
    int indexer = zipFileDataLength;
    int dataIndexer = indexer - 4;

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

        if (pkSignature == PK_CENTRAL_DIRECTORY_SIGNATURE)
        {
            return;
        };

        dataIndexer--;
        indexer--;
    };
};


void GetEndCentralDirectory(const uint8_t* zipFileData, const int& zipFileDataLength, std::vector<Byte>& outdata)
{
    int indexer = zipFileDataLength;
    int dataIndexer = indexer - 4;

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


int main()
{
    const wchar_t* zipFilepath = L"TestZip1.zip";

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

    uint8_t* zipFileBuffer = new uint8_t[zipFileSize + 0] { 0 };

    fileStream.read((char*)&zipFileBuffer[0], zipFileSize);

    std::vector<std::vector<Byte>> files;

    int pkSignature = 0;

    int indexer = 0;

    int fileIndexer = -1;


    while (indexer < zipFileSize)
    {
        pkSignature = zipFileBuffer[indexer + 0];
        pkSignature <<= 8;
        pkSignature |= zipFileBuffer[indexer + 1];
        pkSignature <<= 8;
        pkSignature |= zipFileBuffer[indexer + 2];
        pkSignature <<= 8;
        pkSignature |= zipFileBuffer[indexer + 3];

        if (pkSignature == PK_FILE_HEADER_SIGNATURE)
        {
            files.push_back(std::vector<Byte>());
            fileIndexer++;
        };

        files[fileIndexer].emplace_back(zipFileBuffer[indexer]);

        indexer++;
    };


    std::vector<Byte> endCentralDirectoryOut;
    GetEndCentralDirectory(zipFileBuffer, zipFileSize, endCentralDirectoryOut);

    int CentralDirectoryOffset = (endCentralDirectoryOut[16] |
                                  endCentralDirectoryOut[17] << 8 |
                                  endCentralDirectoryOut[18] << 16 |
                                  endCentralDirectoryOut[19] << 24);

    int sig = (zipFileBuffer[20276] |
               zipFileBuffer[20277] << 8 |
               zipFileBuffer[20278] << 16 |
               zipFileBuffer[20279] << 24);


    if (sig == PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN)
    {
        int s = 0;
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


    delete[] zipFileBuffer;
    zipFileBuffer = nullptr;
};