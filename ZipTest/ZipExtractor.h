#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

#include "deflate.h"


namespace ZipExtractor
{
    // A signature for a zip file header, Equivalent  to PK0304
    constexpr int PK_FILE_HEADER_SIGNATURE = 0x504b0304;

    // A signature for a zip file Central directory
    constexpr int PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN = 0x2014B50;

    // A signature for a zip file End central directory
    constexpr int PK_END_OF_CENTRAL_DIRECTORY = 0x504b0506;


    // A compression method used by the Zip file to compresse the file's contents.
    // Most of the time zip uses the DEFLATE algorithm to compress the files
    enum class CompressionMethod : short
    {
        // No ecnryption is used
        None = 0,

        Shrunk = 1,

        LZW = 1,

        ReducedWithCompressionFactor1 = 2,
        ReducedWithCompressionFactor2 = 3,
        ReducedWithCompressionFactor3 = 4,
        ReducedWithCompressionFactor4 = 5,

        Imploded = 6,

        Reserved_1 = 7,

        // Zip used DEFLATE to compress the file
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


    // An encryption type used by zip to encrypt the data.
    // Unfortunately this extractor doesn't yet implement an encryption-decryption system
    enum class ZipEncryption
    {
        // No encryption was used 
        None = 0,

        // Zip file was encrypted using WinZip Advanced Encryption Standard to encrypt the files
        AES = 1,

        // Zip file was encrypted using the Proprietary PKZip ZipCrypto encryption
        ZipCrypto = 2,
    };



    // A namespace contaning utilities for the ZipExtractor
    namespace Utilities
    {

        /// <summary>
        /// Get the compression method used to compress the files
        /// </summary>
        /// <remarks> Before retrieving the compression type the encryption type must be known before-hand </remarks>
        /// <param name="encryptionType"> Encryption type used to encrypt the zip </param>
        /// <param name="fileHeaderPointer"> A pointer to the file header </param>
        /// <param name="extraField"> Extra field inside the FileHeader </param>
        /// <returns></returns>
        CompressionMethod GetCompressionMethod(ZipEncryption encryptionType, uint8_t* const fileHeaderPointer, uint8_t* extraField)
        {
            // Compression method is store as a 2 byte value inside the File header
            unsigned short compressionMethod = 0;

            // Different operations are performed to find the compression method depending on the encryption type 

            // If no encryption was used...
            if (encryptionType == ZipEncryption::None)
            {
                // Get compression method from File header
                compressionMethod = (fileHeaderPointer[8] |
                                     fileHeaderPointer[9] << 8);
            }
            // If AES encryption was used...
            else if (encryptionType == ZipEncryption::AES)
            {
                // Get compression method from the extra field 
                compressionMethod = (extraField[static_cast<short>(9)] |
                                     extraField[static_cast<short>(10)] << 8);
            };

            // Return the compression method and cast the 2 byte value to the CompressionMethod enum
            return static_cast<CompressionMethod>(compressionMethod);
        };

        /// <summary>
        /// Gets the filename from the File header
        /// </summary>
        /// <param name="fileHeaderPointer"> The pointer to the file header </param>
        /// <param name="filenameLength"> The filename's length </param>
        /// <param name="outString"> An out variable that will contain the filename as an std::string </param>
        void GetFilenname(uint8_t* fileHeaderPointer, size_t filenameLength, std::string& outString)
        {
            char* filenamePointer = reinterpret_cast<char*>(&fileHeaderPointer[30]);

            // Assign the filename from the Filename pointer plus the size of the filename
            outString.assign(filenamePointer, filenamePointer + filenameLength);
        };


        /// <summary>
        /// Gets the encryption type used to encrypt this zip file
        /// </summary>
        /// <remarks> At the moment the only encryption types supported are; None, and AES </remarks>
        /// <param name="centralDirectory">  </param>
        /// <returns></returns>
        ZipExtractor::ZipEncryption GetEncryptionType(const std::vector<uint8_t>& centralDirectory)
        {
            // Get the general purpose bit flag from the central directory
            const unsigned short generalPurposeBitFlag = (centralDirectory[8] |
                                                          centralDirectory[9] << 8);

            // If the first bit inside the flag is marked, then AES was used 
            bool isEncrypted = generalPurposeBitFlag & 1 << 0;


            // Because AES isn't actually implemented by the PKZip standard it uses a different "convention" to indicate which encryption was used.
            // The compression method inside the central directory will be set to 99 if AES was used
            const unsigned short compressionMethod = (centralDirectory[10] |
                                                      centralDirectory[11] << 8);

            // crc32 value will be set to 0 if AES is used
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


        bool IsDirectory(const std::vector<uint8_t>& centralDirectory)
        {
            const unsigned int compressedSize = (centralDirectory[20] |
                                                 centralDirectory[21] << 8 |
                                                 centralDirectory[22] << 16 |
                                                 centralDirectory[23] << 24);

            const unsigned int uncompressedSize = (centralDirectory[24] |
                                                   centralDirectory[25] << 8 |
                                                   centralDirectory[26] << 16 |
                                                   centralDirectory[27] << 24);

            if (compressedSize == 0 && uncompressedSize == 0)
                return true;
            else
                return false;
        };

    };



    /// <summary>
    /// Reads a zip file from given path and outputs a Vector containg the data inside the zip
    /// </summary>
    /// <param name="zipFilepath"> A filepath to the zip </param>
    /// <param name="zipFileBufferOut"> An output buffer that will contain the zip file's data inside </param>
    void ReadZipFile(std::string zipFilepath, std::vector<uint8_t>& zipFileBufferOut)
    {
        std::ifstream fileStream(zipFilepath, std::ios::binary);

        // Simple file validation
        if (fileStream.is_open() == false)
        {
            throw std::exception("Error opening file");
        };

        if (fileStream.good() == false)
        {
            throw std::exception("File error");
        };


        // Find the size of the zip file
        uintmax_t zipFileBufferLength = std::filesystem::file_size(zipFilepath);

        // Reserve some space inside the out buffer
        zipFileBufferOut.reserve(zipFileBufferLength);

        // Store the zip file data inside an intermediate buffer 
        uint8_t* zipFileBuffer = new uint8_t[zipFileBufferLength] { 0 };

        // Read the zip file's contents into zipFileBuffer 
        fileStream.read((char*)&zipFileBuffer[0], zipFileBufferLength);

        // copy the data inside zipFileBuffer to zipFileBufferOut
        zipFileBufferOut.assign(zipFileBuffer, zipFileBuffer + zipFileBufferLength);

        // Free zipFileBuffer
        delete[] zipFileBuffer;
        zipFileBuffer = nullptr;
    };


    /// <summary>
    /// Retrieves the End Central Directory as a buffer
    /// </summary>
    /// <param name="zipFileData"> The zip file's buffer, should contain the entire zip file </param>
    /// <param name="endCentralDirectoryOut"> An out vector that will contain the End Central Drectory buffer </param>
    void GetEndCentralDirectory(std::vector<uint8_t>& const zipFileData, std::vector<uint8_t>& endCentralDirectoryOut)
    {
        // A signature used by PKZip to signifiy an end central directory
        int pkSignature = 0;

        // A pointer to the data inside the zip file buffer, start from the end
        uint8_t* zipFileDataPointer = &zipFileData[zipFileData.size() - 1];

        // A pointer to the "end" of the data "stream" points to the start
        uint8_t const* const zipFileDataPointerEnd = &zipFileData[0];


        // Continue looping while the signature doens't equal the End Central Directory signature
        while (pkSignature != PK_END_OF_CENTRAL_DIRECTORY)
        {
            // A simple validation check so we don't infinitely read data and stop before reading invalid data
            if (zipFileDataPointer == zipFileDataPointerEnd)
            {
                __debugbreak();

                throw std::exception("Reading invalid data");
            };

            // Read the current signature relative to the data pointer
            pkSignature = (*zipFileDataPointer |
                           *(zipFileDataPointer - 1) << 8 |
                           *(zipFileDataPointer - 2) << 16 |
                           *(zipFileDataPointer - 3) << 24);

            // Inset the current byte into the output vector
            endCentralDirectoryOut.insert(endCentralDirectoryOut.begin(), *zipFileDataPointer);

            // Move to next address
            zipFileDataPointer--;
        }

        // After we fnish reading, insert the last 3 unread bytes
        endCentralDirectoryOut.insert(endCentralDirectoryOut.begin(), *(zipFileDataPointer));
        endCentralDirectoryOut.insert(endCentralDirectoryOut.begin(), *(zipFileDataPointer - 1));
        endCentralDirectoryOut.insert(endCentralDirectoryOut.begin(), *(zipFileDataPointer - 2));
    };



    /// <summary>
    /// Get a list of central directories inside the zip file
    /// </summary>
    /// <param name="zipFileData"> The zip file's data buffer </param>
    /// <param name="endCentralDirectory"> The zip file's end central directory </param>
    /// <param name="centralDirectoriesOut"> An output list that will contain a lists of central directories </param>
    void GetCentralDirectories(std::vector<uint8_t>& const zipFileData, const std::vector<uint8_t>& endCentralDirectory, std::vector<std::vector<uint8_t>>& centralDirectoriesOut)
    {
        // Get the offset to the first Central directory
        int centralDirectoryOffset = (endCentralDirectory[16] |
                                      endCentralDirectory[17] << 8 |
                                      endCentralDirectory[18] << 16 |
                                      endCentralDirectory[19] << 24);

        // A pointer to the central directory
        uint8_t* centralDirectoryPointer = &zipFileData[centralDirectoryOffset];

        // A pointer to the end of the last central directory, pointer to the byte just before the End Central Directory
        uint8_t const* const centralDirectoryPointerEnd = &zipFileData[(zipFileData.size() - endCentralDirectory.size()) - 1];

        int centralDirectoySignature = 0;

        // Continue reading data until we reach the end of the last central directory
        while (centralDirectoryPointer != centralDirectoryPointerEnd)
        {
            // Read the signature
            centralDirectoySignature = (*centralDirectoryPointer |
                                        *(centralDirectoryPointer + 1) << 8 |
                                        *(centralDirectoryPointer + 2) << 16 |
                                        *(centralDirectoryPointer + 3) << 24);


            // If the signature matches the central directory signature
            if (centralDirectoySignature == PK_CENTRAL_DIRECTORY_SIGNATURE_LITTLE_ENDIAN)
            {
                // Add a new vector to the output vector
                centralDirectoriesOut.emplace_back(std::vector<uint8_t>());
            };

            // Insert the current byte into the last vector inside the output vector
            (*(centralDirectoriesOut.end() - 1)).emplace_back(*centralDirectoryPointer);

            // Move to next address
            centralDirectoryPointer++;
        };
    };



    /// <summary>
    /// Extract a single folder from the zip file
    /// </summary>
    /// <param name="zipFileData"> The zip file's data buffer </param>
    /// <param name="centralDirectory"> the central directory contaning the folder </param>
    /// <param name="outputFolder"> An path where the output folder will be created </param>
    void ExtractSingleFolder(std::vector<uint8_t>& const zipFileData, const std::vector<uint8_t>& centralDirectory, std::string outputFolder)
    {
        // An offset inside the central directory from where the file header begins
        const int fileHeaderOffset = (centralDirectory[42] |
                                      centralDirectory[43] << 8 |
                                      centralDirectory[44] << 16 |
                                      centralDirectory[45] << 24);

        // A pointer to the File header
        uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];

        // The length of the folder name 
        const short folderNameLength = (fileHeaderPointer[26] |
                                        fileHeaderPointer[27] << 8);

        // A pointer to the begging of the folder name
        const char* folderNamePointer = reinterpret_cast<char*>(&fileHeaderPointer[30]);


        outputFolder.append("/");
        outputFolder.append(folderNamePointer, folderNameLength);

        // Create the folder
        std::filesystem::create_directories(outputFolder);
    };


    /// <summary>
    /// Extracts a single file from inside of the zip
    /// </summary>
    /// <param name="zipFileData"> The zip file buffer </param>
    /// <param name="centralDirectory"> A central directory of the file </param>
    /// <param name="encryptionType"> An encryption type used to encrypt the zip </param>
    /// <param name="outputFolder"> An output path to which the file will be extracted </param>
    void ExtractSingleFile(std::vector<uint8_t>& const zipFileData, const std::vector<uint8_t>& centralDirectory, ZipEncryption encryptionType, std::string outputFolder)
    {
        // An offset to the File header
        const int fileHeaderOffset = (centralDirectory[42] |
                                      centralDirectory[43] << 8 |
                                      centralDirectory[44] << 16 |
                                      centralDirectory[45] << 24);

        // A pointer to the File header
        uint8_t* const fileHeaderPointer = &zipFileData[fileHeaderOffset];


        // The length of the file name
        const short filenameLength = (fileHeaderPointer[26] |
                                      fileHeaderPointer[27] << 8);

        // The length of the extras field
        const short extraFieldLength = (fileHeaderPointer[28] |
                                        fileHeaderPointer[29] << 8);

        uint8_t* extraField = nullptr;

        // If theres extra data for this zip file
        if (extraFieldLength != 0)
        {
            // Read the Extras field
            extraField = new uint8_t[extraFieldLength] { 0 };
            memcpy_s(extraField, extraFieldLength, &fileHeaderPointer[static_cast<size_t>(filenameLength) + 30], extraFieldLength);
        };


        // Get compression method used to compress this file 
        CompressionMethod compressionMethod = Utilities::GetCompressionMethod(encryptionType, fileHeaderPointer, extraField);

        // Size of the file after compression
        const unsigned int compressedSize = (fileHeaderPointer[18] |
                                             fileHeaderPointer[19] << 8 |
                                             fileHeaderPointer[20] << 16 |
                                             fileHeaderPointer[21] << 24);

        // Size of the file pre-compression
        const unsigned int uncompressedSize = (fileHeaderPointer[22] |
                                               fileHeaderPointer[23] << 8 |
                                               fileHeaderPointer[24] << 16 |
                                               fileHeaderPointer[25] << 24);


        // Different extraction operations are performed depending on the compression type
        switch (compressionMethod)
        {
            // If DEFLATE compression was used
            case CompressionMethod::Deflated:
            {
                // If the file isn't encrypted
                if (encryptionType == ZipEncryption::None)
                {
                    // A pointer to the file's data
                    uint8_t* fileHeaderDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];

                    // A buffer contaning the uncompressed data.
                    // Because we are using DEFLATE we must add a header so the decompression algorithm knows how to handle this data
                    uint8_t* fileDataBuffer = new uint8_t[static_cast<size_t>(uncompressedSize) + 2] { 0 };
                    // This is why we add the 2 bytes here
                    fileDataBuffer[0] = 0x78;
                    fileDataBuffer[1] = 0xDA;

                    // Copy the data from the zipfile into fileDataBuffer 
                    memcpy_s(fileDataBuffer + 2, uncompressedSize, fileHeaderDataPointer, uncompressedSize);

                    // A buffer that will store the uncompressed data
                    uint8_t* uncompressedFileData = new uint8_t[uncompressedSize] { 0 };

                    // The file size after decompression
                    uLong uncompressedFileSize = static_cast<uLong>(uncompressedSize);

                    // Decompress the file
                    int result = uncompress(uncompressedFileData, &uncompressedFileSize, fileDataBuffer, compressedSize + 2);

                    // Get the filename
                    std::string filename;
                    Utilities::GetFilenname(fileHeaderPointer, filenameLength, filename);

                    // Append the filename to the output folder
                    outputFolder.append("/");
                    outputFolder.append(filename);


                    // Write the decompressed file contents onto disk
                    std::ofstream output(outputFolder, std::ios::binary);

                    output.write(reinterpret_cast<char*>(&uncompressedFileData[0]), uncompressedFileSize);
                    output.close();


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


            // If no compression was used
            case CompressionMethod::None:
            {
                // If the file isn't encrypted
                if (encryptionType == ZipEncryption::None)
                {
                    // A pointer to the file's data
                    uint8_t* fileHeaderDataPointer = &fileHeaderPointer[30 + filenameLength + extraFieldLength];

                    // Get filename
                    std::string filename;
                    Utilities::GetFilenname(fileHeaderPointer, filenameLength, filename);

                    // Append the filename to the output folder path
                    outputFolder.append("/");
                    outputFolder.append(filename);

                    // Write file to disk
                    std::ofstream output(outputFolder, std::ios::binary);

                    output.write(reinterpret_cast<char*>(&fileHeaderDataPointer[0]), uncompressedSize);
                    output.close();
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


    /// <summary>
    /// Extract the entire zip's contents 
    /// </summary>
    /// <param name="outputPath"> An output path to where the contents will be extracted </param>
    /// <param name="zipFileBuffer"> The zip's file buffer </param>
    /// <param name="centralDirectories"> The list of central directories </param>
    void ExtractZip(const std::string& outputPath, std::vector<uint8_t>& const zipFileBuffer, const std::vector<std::vector<uint8_t>>& centralDirectories)
    {
        // Go through every central directory
        for (const std::vector<uint8_t>& centralDirectory : centralDirectories)
        {
            // Check if central directory is encrypted
            ZipExtractor::ZipEncryption encryptionType = Utilities::GetEncryptionType(centralDirectory);

            if (encryptionType == ZipExtractor::ZipEncryption::AES)
                throw std::exception("AES encryption isn't supported, yet.");

            // Check if central directory is a folder or file
            if (Utilities::IsDirectory(centralDirectory) == true)
                ZipExtractor::ExtractSingleFolder(zipFileBuffer, centralDirectory, outputPath);
            else
                ZipExtractor::ExtractSingleFile(zipFileBuffer, centralDirectory, encryptionType, outputPath);
        };

    };


};