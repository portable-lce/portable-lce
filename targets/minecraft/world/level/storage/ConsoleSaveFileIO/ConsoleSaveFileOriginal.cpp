#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOriginal.h"

#include <assert.h>
#include <wchar.h>

#include <algorithm>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <format>
#include <vector>

#include "platform/PlatformTypes.h"
#include "minecraft/GameEnums.h"
#include "minecraft/BuildVer.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "java/File.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/System.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/server/MinecraftServer.h"
#include "minecraft/server/level/ServerLevel.h"
#include "minecraft/world/level/chunk/storage/RegionFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "platform/storage/storage.h"
#include "platform/fs/fs.h"


ConsoleSaveFileOriginal::ConsoleSaveFileOriginal(
    const std::string& fileName, void* pvSaveData /*= nullptr*/,
    unsigned int initialFileSize /*= 0*/, bool forceCleanSave /*= false*/,
    ESavePlatform plat /*= SAVE_FILE_PLATFORM_LOCAL*/)
    : saveBuffer(MAX_SAVE_SIZE) {
    m_fileName = fileName;

    unsigned int fileSize = initialFileSize;

    // Load a save from the game rules
    bool bLevelGenBaseSave = false;
    LevelGenerationOptions* levelGen = gameServices().getLevelGenerationOptions();
    if (pvSaveData == nullptr && levelGen != nullptr &&
        levelGen->requiresBaseSave()) {
        pvSaveData = levelGen->getBaseSaveData(fileSize);
        if (pvSaveData && fileSize != 0) bLevelGenBaseSave = true;
    }

    if (pvSaveData == nullptr || fileSize == 0)
        fileSize = PlatformStorage.GetSaveSize();

    if (forceCleanSave) fileSize = 0;

    if (fileSize > 0) {
        if (pvSaveData != nullptr) {
            memcpy(pvSaveMem, pvSaveData, fileSize);
            if (bLevelGenBaseSave) {
                levelGen->deleteBaseSaveData();
            }
        } else {
            unsigned int storageLength;
            PlatformStorage.GetSaveData(pvSaveMem, &storageLength);
            Log::info("Filesize - %d, Adjusted size - %d\n", fileSize,
                            storageLength);
            fileSize = storageLength;
        }
        void* pvSourceData = pvSaveMem;
        int compressed = *(int*)pvSourceData;
        if (compressed == 0) {
            unsigned int decompSize = *((int*)pvSourceData + 1);
            if (isLocalEndianDifferent(plat)) System::ReverseULONG(&decompSize);

            // An invalid save, so clear the memory and start from scratch
            if (decompSize == 0) {
                // 4J Stu - Saves created between 2/12/2011 and 7/12/2011
                // will have this problem
                Log::info("Invalid save data format\n");
                std::memset(pvSourceData, 0, fileSize);
                // Clear the first 8 bytes that reference the header
                header.WriteHeader(pvSourceData);
            } else {
                assert(decompSize <= MAX_SAVE_SIZE);
                unsigned char* buf = new unsigned char[decompSize];
                Compression::getCompression()->SetDecompressionType(
                    plat);  // if this save is from another platform, set the
                            // correct decompression type
                Compression::getCompression()->Decompress(
                    buf, &decompSize, (unsigned char*)pvSourceData + 8,
                    fileSize - 8);
                Compression::getCompression()->SetDecompressionType(
                    SAVE_FILE_PLATFORM_LOCAL);  // and then set the
                                                // decompression back to the
                                                // local machine's standard type
                memcpy(pvSaveMem, buf, decompSize);
                delete[] buf;
            }
        }

        header.ReadHeader(pvSaveMem, plat);

    } else {
        // Clear the first 8 bytes that reference the header
        header.WriteHeader(pvSaveMem);
    }
}

ConsoleSaveFileOriginal::~ConsoleSaveFileOriginal() = default;

// Add the file to our table of internal files if not already there
// Open our actual save file ready for reading/writing, and the set the file
// pointer to the start of this file
FileEntry* ConsoleSaveFileOriginal::createFile(
    const ConsoleSavePath& fileName) {
    LockSaveAccess();
    FileEntry* file = header.AddFile(fileName.getName());
    ReleaseSaveAccess();

    return file;
}

void ConsoleSaveFileOriginal::deleteFile(FileEntry* file) {
    if (file == nullptr) return;

    LockSaveAccess();

    unsigned int numberOfBytesRead = 0;
    unsigned int numberOfBytesWritten = 0;

    const int bufferSize = 4096;
    int amountToRead = bufferSize;
    std::uint8_t buffer[bufferSize];
    unsigned int bufferDataSize = 0;

    char* readStartOffset =
        (char*)pvSaveMem + file->data.startOffset + file->getFileSize();

    char* writeStartOffset = (char*)pvSaveMem + file->data.startOffset;

    char* endOfDataOffset = (char*)pvSaveMem + header.GetStartOfNextData();

    while (true) {
        // Fill buffer from file
        if (readStartOffset + bufferSize > endOfDataOffset) {
            amountToRead = (int)(endOfDataOffset - readStartOffset);
        } else {
            amountToRead = bufferSize;
        }

        if (amountToRead == 0) break;

        memcpy(buffer, readStartOffset, amountToRead);
        numberOfBytesRead = amountToRead;

        bufferDataSize = amountToRead;
        readStartOffset += numberOfBytesRead;

        // Write buffer to file
        memcpy((void*)writeStartOffset, buffer, bufferDataSize);
        numberOfBytesWritten = bufferDataSize;

        writeStartOffset += numberOfBytesWritten;
    }

    header.RemoveFile(file);

    finalizeWrite();

    ReleaseSaveAccess();
}

void ConsoleSaveFileOriginal::setFilePointer(FileEntry* file,
                                             unsigned int distanceToMove,
                                             SaveFileSeekOrigin seekOrigin) {
    LockSaveAccess();

    switch (seekOrigin) {
        case SaveFileSeekOrigin::Current:
            file->currentFilePointer += distanceToMove;
            break;
        case SaveFileSeekOrigin::End:
            file->currentFilePointer =
                file->data.startOffset + file->getFileSize() + distanceToMove;
            break;
        case SaveFileSeekOrigin::Begin:
        default:
            file->currentFilePointer = file->data.startOffset + distanceToMove;
            break;
    }

    ReleaseSaveAccess();
}

// If this file needs to grow, move the data after along
void ConsoleSaveFileOriginal::PrepareForWrite(
    FileEntry* file, unsigned int nNumberOfBytesToWrite) {
    int bytesToGrowBy = ((file->currentFilePointer - file->data.startOffset) +
                         nNumberOfBytesToWrite) -
                        file->getFileSize();
    if (bytesToGrowBy <= 0) return;

    // 4J Stu - Not forcing a minimum size, it is up to the caller to write data
    // in sensible amounts This lets us keep some of the smaller files small
    // if( bytesToGrowBy < 1024 )
    //	bytesToGrowBy = 1024;

    // Move all the data beyond us
    MoveDataBeyond(file, bytesToGrowBy);

    // Update our length
    if (file->data.length < 0) file->data.length = 0;
    file->data.length += bytesToGrowBy;

    // Write the header with the updated data
    finalizeWrite();
}

bool ConsoleSaveFileOriginal::writeFile(FileEntry* file, const void* lpBuffer,
                                        unsigned int nNumberOfBytesToWrite,
                                        unsigned int* lpNumberOfBytesWritten) {
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    LockSaveAccess();

    PrepareForWrite(file, nNumberOfBytesToWrite);

    char* writeStartOffset = (char*)pvSaveMem + file->currentFilePointer;
    // printf("Write: pvSaveMem = %0xd, currentFilePointer = %d,
    // writeStartOffset = %0xd\n", pvSaveMem, file->currentFilePointer,
    // writeStartOffset);

    memcpy((void*)writeStartOffset, lpBuffer, nNumberOfBytesToWrite);
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;

    if (file->data.length < 0) file->data.length = 0;

    file->currentFilePointer += *lpNumberOfBytesWritten;

    // printf("Wrote %d bytes to %s, new file pointer is %I64d\n",
    // *lpNumberOfBytesWritten, file->data.filename, file->currentFilePointer);

    file->updateLastModifiedTime();

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileOriginal::zeroFile(FileEntry* file,
                                       unsigned int nNumberOfBytesToWrite,
                                       unsigned int* lpNumberOfBytesWritten) {
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    LockSaveAccess();

    PrepareForWrite(file, nNumberOfBytesToWrite);

    char* writeStartOffset = (char*)pvSaveMem + file->currentFilePointer;
    // printf("Write: pvSaveMem = %0xd, currentFilePointer = %d,
    // writeStartOffset = %0xd\n", pvSaveMem, file->currentFilePointer,
    // writeStartOffset);

    memset((void*)writeStartOffset, 0, nNumberOfBytesToWrite);
    *lpNumberOfBytesWritten = nNumberOfBytesToWrite;

    if (file->data.length < 0) file->data.length = 0;

    file->currentFilePointer += *lpNumberOfBytesWritten;

    // printf("Wrote %d bytes to %s, new file pointer is %I64d\n",
    // *lpNumberOfBytesWritten, file->data.filename, file->currentFilePointer);

    file->updateLastModifiedTime();

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileOriginal::readFile(FileEntry* file, void* lpBuffer,
                                       unsigned int nNumberOfBytesToRead,
                                       unsigned int* lpNumberOfBytesRead) {
    unsigned int actualBytesToRead;
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    LockSaveAccess();

    char* readStartOffset = (char*)pvSaveMem + file->currentFilePointer;
    // printf("Read: pvSaveMem = %0xd, currentFilePointer = %d, readStartOffset
    // = %0xd\n", pvSaveMem, file->currentFilePointer, readStartOffset);

    assert(nNumberOfBytesToRead <= file->getFileSize());

    actualBytesToRead = nNumberOfBytesToRead;
    if (file->currentFilePointer + nNumberOfBytesToRead >
        file->data.startOffset + file->data.length) {
        actualBytesToRead = (file->data.startOffset + file->data.length) -
                            file->currentFilePointer;
    }

    memcpy(lpBuffer, readStartOffset, actualBytesToRead);

    *lpNumberOfBytesRead = actualBytesToRead;

    file->currentFilePointer += *lpNumberOfBytesRead;

    // printf("Read %d bytes from %s, new file pointer is %I64d\n",
    // *lpNumberOfBytesRead, file->data.filename, file->currentFilePointer);

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileOriginal::closeHandle(FileEntry* file) {
    LockSaveAccess();
    finalizeWrite();
    ReleaseSaveAccess();

    return true;
}

void ConsoleSaveFileOriginal::finalizeWrite() {
    LockSaveAccess();
    header.WriteHeader(pvSaveMem);
    ReleaseSaveAccess();
}

void ConsoleSaveFileOriginal::MoveDataBeyond(
    FileEntry* file, unsigned int nNumberOfBytesToWrite) {
    unsigned int numberOfBytesRead = 0;
    unsigned int numberOfBytesWritten = 0;

    const unsigned int bufferSize = 4096;
    unsigned int amountToRead = bufferSize;
    // assert( nNumberOfBytesToWrite <= bufferSize );
    static std::uint8_t buffer1[bufferSize];
    static std::uint8_t buffer2[bufferSize];
    unsigned int buffer1Size = 0;
    unsigned int buffer2Size = 0;

    assert(header.GetFileSize() + nNumberOfBytesToWrite <= MAX_SAVE_SIZE);

    // This is the start of where we want the space to be, and the start of the
    // data that we need to move
    char* spaceStartOffset =
        (char*)pvSaveMem + file->data.startOffset + file->getFileSize();

    // This is the end of where we want the space to be
    char* spaceEndOffset = spaceStartOffset + nNumberOfBytesToWrite;

    // This is the current end of the data that we want to move
    char* beginEndOfDataOffset = (char*)pvSaveMem + header.GetStartOfNextData();

    // This is where the end of the data is going to be
    char* finishEndOfDataOffset = beginEndOfDataOffset + nNumberOfBytesToWrite;

    // This is where we are going to read from (with the amount we want to read
    // subtracted before we read)
    char* readStartOffset = beginEndOfDataOffset;

    // This is where we can safely write to (with the amount we want write
    // subtracted before we write)
    char* writeStartOffset = finishEndOfDataOffset;

    // printf("\n******* MOVEDATABEYOND *******\n");
    // printf("Space start: %d, space end: %d\n", spaceStartOffset - (char
    // *)pvSaveMem, spaceEndOffset - (char *)pvSaveMem); printf("Current end of
    // data: %d, new end of data: %d\n", beginEndOfDataOffset - (char
    // *)pvSaveMem, finishEndOfDataOffset - (char *)pvSaveMem);

    // Optimisation for things that are being moved in whole region file sector
    // (4K chunks). We could generalise this a bit more but seems safest at the
    // moment to identify this particular type of move and code explicitly for
    // this situation
    if ((nNumberOfBytesToWrite & 4095) == 0) {
        if (nNumberOfBytesToWrite > 0) {
            // Get addresses for start & end of the region we are copying from
            // as uintptr_t, for easier maths
            uintptr_t uiFromStart = (uintptr_t)spaceStartOffset;
            uintptr_t uiFromEnd = (uintptr_t)beginEndOfDataOffset;

            // Round both of these values to get 4096 byte chunks that we will
            // need to at least partially move
            uintptr_t uiFromStartChunk = uiFromStart & ~((uintptr_t)4095);
            uintptr_t uiFromEndChunk = (uiFromEnd - 1) & ~((uintptr_t)4095);

            // Loop through all the affected source 4096 chunks, going backwards
            // so we don't overwrite anything we'll need in the future
            for (uintptr_t uiCurrentChunk = uiFromEndChunk;
                 uiCurrentChunk >= uiFromStartChunk; uiCurrentChunk -= 4096) {
                // Establish chunk we'll need to copy
                uintptr_t uiCopyStart = uiCurrentChunk;
                uintptr_t uiCopyEnd = uiCurrentChunk + 4096;
                // Clamp chunk to the bounds of the full region we are trying to
                // copy
                if (uiCopyStart < uiFromStart) {
                    // Needs to be clampged against the start of our region
                    uiCopyStart = uiFromStart;
                }
                if (uiCopyEnd > uiFromEnd) {
                    // Needs to be clamped to the end of our region
                    uiCopyEnd = uiFromEnd;
                }
                memcpy((void*)(uiCopyStart + nNumberOfBytesToWrite),
                       (void*)uiCopyStart, uiCopyEnd - uiCopyStart);
            }
        }
    } else {
        while (true) {
            // Copy buffer 1 to buffer 2
            memcpy(buffer2, buffer1, buffer1Size);
            buffer2Size = buffer1Size;

            // Fill buffer 1 from file
            if ((readStartOffset - bufferSize) < spaceStartOffset) {
                amountToRead = static_cast<unsigned int>(readStartOffset -
                                                         spaceStartOffset);
            } else {
                amountToRead = bufferSize;
            }

            // Push the read point back by the amount of bytes that we are going
            // to read
            readStartOffset -= amountToRead;

            // printf("About to read %u from %d\n", amountToRead,
            // readStartOffset - (char *)pvSaveMem );
            memcpy(buffer1, readStartOffset, amountToRead);
            numberOfBytesRead = amountToRead;

            buffer1Size = amountToRead;

            // Move back the write pointer by the amount of bytes we are going
            // to write
            writeStartOffset -= buffer2Size;

            // Write buffer 2 to file
            if ((writeStartOffset + buffer2Size) <= finishEndOfDataOffset) {
                // printf("About to write %u to %d\n", buffer2Size,
                // writeStartOffset - (char *)pvSaveMem );
                memcpy((void*)writeStartOffset, buffer2, buffer2Size);
                numberOfBytesWritten = buffer2Size;
            } else {
                assert((writeStartOffset + buffer2Size) <=
                       finishEndOfDataOffset);
                numberOfBytesWritten = 0;
            }

            if (numberOfBytesRead == 0) {
                // printf("\n************** MOVE COMPLETED ***************
                // \n\n");
                assert(writeStartOffset == spaceEndOffset);
                break;
            }
        }
    }

    header.AdjustStartOffsets(file, nNumberOfBytesToWrite);
}

bool ConsoleSaveFileOriginal::doesFileExist(ConsoleSavePath file) {
    LockSaveAccess();
    bool exists = header.fileExists(file.getName());
    ReleaseSaveAccess();

    return exists;
}

void ConsoleSaveFileOriginal::Flush(bool autosave, bool updateThumbnail) {
    LockSaveAccess();

    finalizeWrite();

    float fElapsedTime = 0.0f;

    unsigned int fileSize = header.GetFileSize();

    // Assume that the compression will make it smaller so initially attempt to
    // allocate the current file size We add 4 bytes to the start so that we can
    // signal compressed data And another 4 bytes to store the decompressed data
    // size
    unsigned int compLength = fileSize + 8;

    // 4J Stu - Added TU-1 interim

    // Attempt to allocate the required memory
    // We do not own this, it belongs to the PlatformStorage
    std::uint8_t* compData =
        (std::uint8_t*)PlatformStorage.AllocateSaveData(compLength);

    // If we failed to allocate then compData will be nullptr
    // Pre-calculate the compressed data size so that we can attempt to allocate
    // a smaller buffer
    if (compData == nullptr) {
        // Length should be 0 here so that the compression call knows that we
        // want to know the length back
        compLength = 0;

        // Pre-calculate the buffer size required for the compressed data
        // Save the start time
        const auto startTime = std::chrono::steady_clock::now();
        Compression::getCompression()->Compress(nullptr, &compLength, pvSaveMem,
                                                fileSize);
        fElapsedTime = std::chrono::duration<float>(
                           std::chrono::steady_clock::now() - startTime)
                           .count();

        Log::info("Check buffer size: Elapsed time %f\n", fElapsedTime);

        // We add 4 bytes to the start so that we can signal compressed data
        // And another 4 bytes to store the decompressed data size
        compLength = compLength + 8;

        // Attempt to allocate the required memory
        compData = (std::uint8_t*)PlatformStorage.AllocateSaveData(compLength);
    }

    if (compData != nullptr) {
        // Re-compress all save data before we save it to disk
        // Save the start time
        const auto startTime = std::chrono::steady_clock::now();
        Compression::getCompression()->Compress(compData + 8, &compLength,
                                                pvSaveMem, fileSize);
        fElapsedTime = std::chrono::duration<float>(
                           std::chrono::steady_clock::now() - startTime)
                           .count();

        Log::info("Compress: Elapsed time %f\n", fElapsedTime);

        std::fill_n(compData, 8, std::uint8_t{0});
        int saveVer = 0;
        memcpy(compData, &saveVer, sizeof(int));
        memcpy(compData + 4, &fileSize, sizeof(int));

        Log::info("Save data compressed from %d to %d\n", fileSize,
                        compLength);

        std::uint8_t* pbThumbnailData = nullptr;
        unsigned int dwThumbnailDataSize = 0;

        std::uint8_t* pbDataSaveImage = nullptr;
        unsigned int dwDataSizeSaveImage = 0;

#ifdef _WINDOWS64
        gameServices().getSaveThumbnail(&pbThumbnailData, &dwThumbnailDataSize,
                             &pbDataSaveImage, &dwDataSizeSaveImage);
#endif

        std::uint8_t bTextMetadata[88] = {};

        int64_t seed = 0;
        bool hasSeed = false;
        if (MinecraftServer::getInstance() != nullptr &&
            MinecraftServer::getInstance()->levels[0] != nullptr) {
            seed = MinecraftServer::getInstance()
                       ->levels[0]
                       ->getLevelData()
                       ->getSeed();
            hasSeed = true;
        }

        int iTextMetadataBytes = gameServices().createImageTextData(
            bTextMetadata, seed, hasSeed,
            gameServices().getGameHostOption(eGameHostOption_All),
            Minecraft::GetInstance()->getCurrentTexturePackId());

        int32_t saveOrCheckpointId = 0;
        bool validSave =
            PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);
#ifdef _WINDOWS64
        // set the icon and save image
        PlatformStorage.SetSaveImages(pbThumbnailData, dwThumbnailDataSize,
                                      pbDataSaveImage, dwDataSizeSaveImage,
                                      bTextMetadata, iTextMetadataBytes);
        Log::info("Save thumbnail size %d\n", dwThumbnailDataSize);

        // save the data
        PlatformStorage.SaveSaveData(
            &ConsoleSaveFileOriginal::SaveSaveDataCallback, this);
#ifndef _CONTENT_PACKAGE
        if (gameServices().debugSettingsOn()) {
            if (gameServices().getWriteSavesToFolderEnabled()) {
                DebugFlushToFile(compData, compLength + 8);
            }
        }
#endif
        ReleaseSaveAccess();
#else
        ReleaseSaveAccess();
#endif
    } else {
        // We have failed to allocate the memory required to save this file. Now
        // what?
        ReleaseSaveAccess();
    }
}

#ifdef _WINDOWS64

int ConsoleSaveFileOriginal::SaveSaveDataCallback(void* lpParam, bool bRes) {
    ConsoleSaveFile* pClass = (ConsoleSaveFile*)lpParam;

    return 0;
}

#endif

#ifndef _CONTENT_PACKAGE
void ConsoleSaveFileOriginal::DebugFlushToFile(
    void* compressedData /*= nullptr*/,
    unsigned int compressedDataSize /*= 0*/) {
    LockSaveAccess();

    finalizeWrite();

    unsigned int fileSize = header.GetFileSize();

    unsigned int numberOfBytesWritten = 0;
    File targetFileDir("Saves");

    if (!targetFileDir.exists()) targetFileDir.mkdir();

    char* fileName = new char[XCONTENT_MAX_FILENAME_LENGTH + 1];

    std::time_t now = std::time(nullptr);
    std::tm t = *std::gmtime(&now);

    // 14 chars for the digits
    // 11 chars for the separators + suffix
    // 25 chars total
    std::string cutFileName = m_fileName;
    if (m_fileName.length() > XCONTENT_MAX_FILENAME_LENGTH - 25) {
        cutFileName = m_fileName.substr(0, XCONTENT_MAX_FILENAME_LENGTH - 25);
    }
    snprintf(fileName, XCONTENT_MAX_FILENAME_LENGTH + 1,
             "\\v%04d-%s%02d.%02d.%02d.%02d.%02d.mcs", VER_PRODUCTBUILD,
             cutFileName.c_str(), t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
             t.tm_sec);

    const std::string outputPath =
        targetFileDir.getPath() + std::string(fileName);
    bool writeSucceeded = false;

    if (compressedData != nullptr && compressedDataSize > 0) {
        writeSucceeded = PlatformFilesystem.writeFile(
            outputPath, compressedData, compressedDataSize);
        numberOfBytesWritten = writeSucceeded ? compressedDataSize : 0;
        assert(numberOfBytesWritten == compressedDataSize);
    } else {
        writeSucceeded =
            PlatformFilesystem.writeFile(outputPath, pvSaveMem, fileSize);
        numberOfBytesWritten = writeSucceeded ? fileSize : 0;
        assert(numberOfBytesWritten == fileSize);
    }

    delete[] fileName;

    ReleaseSaveAccess();
}
#endif

unsigned int ConsoleSaveFileOriginal::getSizeOnDisk() {
    return header.GetFileSize();
}

std::string ConsoleSaveFileOriginal::getFilename() { return m_fileName; }

std::vector<FileEntry*>* ConsoleSaveFileOriginal::getFilesWithPrefix(
    const std::string& prefix) {
    return header.getFilesWithPrefix(prefix);
}

std::vector<FileEntry*>* ConsoleSaveFileOriginal::getRegionFilesByDimension(
    unsigned int dimensionIndex) {
    return nullptr;
}

int ConsoleSaveFileOriginal::getSaveVersion() {
    return header.getSaveVersion();
}

int ConsoleSaveFileOriginal::getOriginalSaveVersion() {
    return header.getOriginalSaveVersion();
}

void ConsoleSaveFileOriginal::LockSaveAccess() { m_lock.lock(); }

void ConsoleSaveFileOriginal::ReleaseSaveAccess() { m_lock.unlock(); }

ESavePlatform ConsoleSaveFileOriginal::getSavePlatform() {
    return header.getSavePlatform();
}

bool ConsoleSaveFileOriginal::isSaveEndianDifferent() {
    return header.isSaveEndianDifferent();
}

void ConsoleSaveFileOriginal::setLocalPlatform() { header.setLocalPlatform(); }

void ConsoleSaveFileOriginal::setPlatform(ESavePlatform plat) {
    header.setPlatform(plat);
}

std::endian ConsoleSaveFileOriginal::getSaveEndian() {
    return header.getSaveEndian();
}

std::endian ConsoleSaveFileOriginal::getLocalEndian() {
    return header.getLocalEndian();
}

void ConsoleSaveFileOriginal::setEndian(std::endian endian) {
    header.setEndian(endian);
}

bool ConsoleSaveFileOriginal::isLocalEndianDifferent(ESavePlatform plat) {
    return getLocalEndian() != header.getEndian(plat);
}

void ConsoleSaveFileOriginal::ConvertRegionFile(File sourceFile) {
    unsigned int numberOfBytesWritten = 0;
    unsigned int numberOfBytesRead = 0;

    RegionFile sourceRegionFile(this, &sourceFile);

    for (unsigned int x = 0; x < 32; ++x) {
        for (unsigned int z = 0; z < 32; ++z) {
            DataInputStream* dis =
                sourceRegionFile.getChunkDataInputStream(x, z);

            if (dis) {
                std::vector<uint8_t> inData(1024 * 1024);
                int read = dis->read(inData);
                dis->close();
                dis->deleteChildStream();
                delete dis;

                DataOutputStream* dos =
                    sourceRegionFile.getChunkDataOutputStream(x, z);
                dos->write(inData, 0, read);

                dos->close();
                dos->deleteChildStream();
                delete dos;
                // vector cleans up automatically
            }
        }
    }
    sourceRegionFile
        .writeAllOffsets();  // saves all the endian swapped offsets back out to
                             // the file (not all of these are written in the
                             // above processing).
}

void ConsoleSaveFileOriginal::ConvertToLocalPlatform() {
    if (getSavePlatform() == SAVE_FILE_PLATFORM_LOCAL) {
        // already in the correct format
        return;
    }
    // convert each of the region files to the local platform
    std::vector<FileEntry*>* allFilesInSave =
        getFilesWithPrefix(std::string(""));
    for (auto it = allFilesInSave->begin(); it < allFilesInSave->end(); ++it) {
        FileEntry* fe = *it;
        std::string fName(fe->data.filename);
        std::string suffix(".mcr");
        if (fName.compare(fName.length() - suffix.length(), suffix.length(),
                          suffix) == 0) {
            Log::info("Processing a region file: %s\n", fName.c_str());
            ConvertRegionFile(File(fe->data.filename));
        } else {
            Log::info("%s is not a region file, ignoring\n",
                            fName.c_str());
        }
    }

    setLocalPlatform();  // set the platform of this save to the local platform,
                         // now that it's been coverted
}

void* ConsoleSaveFileOriginal::getWritePointer(FileEntry* file) {
    return (char*)pvSaveMem + file->currentFilePointer;
    ;
}
