#include <fstream>
#include <filesystem>
#include <iostream>


constexpr int PK_SIGNATURE = 0x504b0304;


const wchar_t* GetLocalZipFileName(std::vector<uint8_t>& const zipFileData)
{
    const short filenameLength = zipFileData[26];

    wchar_t* filename = new wchar_t[filenameLength + 1] { 0 };

    for (int a = 0; a < filenameLength; a++)
    {
        filename[a] = zipFileData[30 + a];
    };

    return filename;
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

    wchar_t* buffer = new wchar_t[zipFileSize];
    memset(buffer, 0, zipFileSize);

    fileStream.read(buffer, zipFileSize);


    std::vector<std::vector<uint8_t>> files;

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
            files.push_back(std::vector<uint8_t>());
            fileIndexer++;
        };

        files[fileIndexer].emplace_back(buffer[indexer]);

        indexer++;
    };


    for (int a = 0; a < files.size(); a++)
    {
        const wchar_t* filename = GetLocalZipFileName(files[a]);
        
        std::wcout << filename << L"\n";

        delete[] filename;
        buffer = nullptr;
    };

    delete[] buffer;
    buffer = nullptr;
};