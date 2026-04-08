#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileSplit.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <chrono>
#include <compare>
#include <ctime>
#include <format>
#include <thread>
#include <utility>

#include "platform/fs/fs.h"
#include "platform/PlatformTypes.h"
#include "minecraft/GameEnums.h"
#include "minecraft/BuildVer.h"
#include "app/common/GameRules/LevelGeneration/LevelGenerationOptions.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "util/Timer.h"
#include "util/StringHelpers.h"
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
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileConverter.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "platform/storage/storage.h"

class ProgressListener;

#define RESERVE_ALLOCATION MEM_RESERVE
#define COMMIT_ALLOCATION MEM_COMMIT

unsigned int ConsoleSaveFileSplit::pagesCommitted = 0;
void* ConsoleSaveFileSplit::pvHeap = nullptr;

ConsoleSaveFileSplit::RegionFileReference::RegionFileReference(
    int index, unsigned int regionIndex, unsigned int length /*=0*/,
    unsigned char* data /*=nullptr*/) {
    fileEntry = new FileEntry();
    fileEntry->currentFilePointer = 0;
    fileEntry->data.length = 0;
    fileEntry->data.regionIndex = regionIndex;
    this->data = 0;
    this->index = index;
    this->dirty = false;
    this->dataCompressed = data;
    this->dataCompressedSize = length;
    this->lastWritten = 0;
}

ConsoleSaveFileSplit::RegionFileReference::~RegionFileReference() {
    free(data);
    delete fileEntry;
}

// Compress from data to dataCompressed. Uses a special compression method that
// is designed just to efficiently store runs of zeros, with little overhead on
// other stuff. Compresed format is a 4 byte uncompressed size, followed by data
// as follows:
//
// Byte value
// Meaning
//
// 1 - 255
// Normal data 0 followed by 1 - 255
// Run of 1 - 255 0s 0 followed by 0, followed by 256 to 65791 (as 2 bytes)
// Run of 256 to 65791 zeros

void ConsoleSaveFileSplit::RegionFileReference::Compress() {
    unsigned char* dataIn = data;
    unsigned char* dataInLast = data + fileEntry->data.length;

    //	std::int64_t startTime = System::currentTimeMillis();

    // One pass through to work out storage space required for compressed data
    unsigned int outputSize = 4;  // 4 bytes required to store the uncompressed
                                  // size for faster decompression
    unsigned int runLength = 0;
    while (dataIn != dataInLast) {
        unsigned char thisByte = *dataIn++;
        if ((thisByte != 0) || (runLength == (65535 + 256))) {
            // We've got a non-zero value, or we've hit our maximum run length.
            // If there was a preceeding run of zeros, encode that nwo
            if (runLength != 0) {
                if (runLength < 256) {
                    // Runs of 1 to 255 encoded as 0 followed by one byte of run
                    // length
                    outputSize += 2;
                } else {
                    // Runs of 256 to 65791 encoded as two 0s followed by two
                    // bytes of run length - 256
                    outputSize += 4;
                }
                // Run is now processed
                runLength = 0;
            }
            // Now handle the current byte
            if (thisByte == 0) {
                runLength++;
            } else {
                // Non-zero, just copy over to output
                outputSize++;
            }
        } else {
            // It's a zero - keep counting size of the run
            runLength++;
        }
    }
    // Handle any outstanding run
    if (runLength != 0) {
        if (runLength < 256) {
            // Runs of 1 to 255 encoded as 0 followed by one byte of run length
            outputSize += 2;
        } else {
            // Runs of 256 to 65791 encoded as two 0s followed by two bytes of
            // run length - 256
            outputSize += 4;
        }
        // Run is now processed
        runLength = 0;
    }

    // Now actually allocate & write the compress data. First 4 bytes store the
    // uncompressed size
    dataCompressed = (unsigned char*)malloc(outputSize);
    *((unsigned int*)dataCompressed) = fileEntry->data.length;
    unsigned char* dataOut = dataCompressed + 4;
    dataIn = data;

    // Now same process as before, but actually writing
    while (dataIn != dataInLast) {
        unsigned char thisByte = *dataIn++;
        if ((thisByte != 0) || (runLength == (65535 + 256))) {
            // We've got a non-zero value, or we've hit our maximum run length.
            // If there was a preceeding run of zeros, encode that nwo
            if (runLength != 0) {
                if (runLength < 256) {
                    // Runs of 1 to 255 encoded as 0 followed by one byte of run
                    // length
                    *dataOut++ = 0;
                    *dataOut++ = runLength;
                } else {
                    // Runs of 256 to 65791 encoded as two 0s followed by two
                    // bytes of run length - 256
                    *dataOut++ = 0;
                    *dataOut++ = 0;
                    unsigned int largeRunLength = runLength - 256;
                    *dataOut++ = (largeRunLength >> 8) & 0xff;
                    *dataOut++ = (largeRunLength) & 0xff;
                }
                // Run is now processed
                runLength = 0;
            }
            // Now handle the current byte
            if (thisByte == 0) {
                runLength++;
            } else {
                // Non-zero, just copy over to output
                *dataOut++ = thisByte;
            }
        } else {
            // It's a zero - keep counting size of the run
            runLength++;
        }
    }
    // Handle any outstanding run
    if (runLength != 0) {
        if (runLength < 256) {
            // Runs of 1 to 255 encoded as 0 followed by one byte of run length
            *dataOut++ = 0;
            *dataOut++ = runLength;
        } else {
            // Runs of 256 to 65791 encoded as two 0s followed by two bytes of
            // run length - 256
            *dataOut++ = 0;
            *dataOut++ = 0;
            unsigned int largeRunLength = runLength - 256;
            *dataOut++ = (largeRunLength >> 8) & 0xff;
            *dataOut++ = (largeRunLength) & 0xff;
        }
        // Run is now processed
        runLength = 0;
    }
    assert((dataOut - dataCompressed) == outputSize);
    dataCompressedSize = outputSize;
    //	std::int64_t endTime = System::currentTimeMillis();
    //	Log::info("Compressing region file 0x%.8x from %d to %d bytes -
    //%dms\n", fileEntry->data.regionIndex, fileEntry->data.length,
    // dataCompressedSize, endTime - startTime);
}

// Decompress from dataCompressed -> data. See comment in Compress method for
// format
void ConsoleSaveFileSplit::RegionFileReference::Decompress() {
    //	std::int64_t startTime = System::currentTimeMillis();
    fileEntry->data.length = *((unsigned int*)dataCompressed);

    // If this is unusually large, then test how big it would be when expanded
    // before trying to allocate. Matching the expanded size is (currently) our
    // means of knowing that this file is ok
    if (fileEntry->data.length > 1 * 1024 * 1024) {
        unsigned int uncompressedSize = 0;
        unsigned char* dataIn = dataCompressed + 4;
        unsigned char* dataInLast = dataCompressed + dataCompressedSize;

        while (dataIn != dataInLast) {
            unsigned char thisByte = *dataIn++;
            if (thisByte == 0) {
                thisByte = *dataIn++;
                if (thisByte == 0) {
                    unsigned int runLength = (*dataIn++) << 8;
                    runLength |= (*dataIn++);
                    runLength += 256;
                    uncompressedSize += runLength;
                } else {
                    unsigned int runLength = thisByte;
                    uncompressedSize += runLength;
                }
            } else {
                uncompressedSize++;
            }
        }

        if (fileEntry->data.length != uncompressedSize) {
            // Treat as if it was an empty region file
            fileEntry->data.length = 0;
            assert(0);
            return;
        }
    }

    data = (unsigned char*)malloc(fileEntry->data.length);
    unsigned char* dataIn = dataCompressed + 4;
    unsigned char* dataInLast = dataCompressed + dataCompressedSize;
    unsigned char* dataOut = data;

    while (dataIn != dataInLast) {
        unsigned char thisByte = *dataIn++;
        if (thisByte == 0) {
            thisByte = *dataIn++;
            if (thisByte == 0) {
                unsigned int runLength = (*dataIn++) << 8;
                runLength |= (*dataIn++);
                runLength += 256;
                for (unsigned int i = 0; i < runLength; i++) {
                    *dataOut++ = 0;
                }
            } else {
                unsigned int runLength = thisByte;
                for (unsigned int i = 0; i < runLength; i++) {
                    *dataOut++ = 0;
                }
            }
        } else {
            *dataOut++ = thisByte;
        }
    }
    // If we failed to correctly decompress, then treat as if it was an empty
    // region file
    if ((dataOut - data) != fileEntry->data.length) {
        free(data);
        fileEntry->data.length = 0;
        data = nullptr;
        assert(0);
    }
    //	std::int64_t endTime = System::currentTimeMillis();
    //	Log::info("Decompressing region file from 0x%.8x %d to %d bytes -
    //%dms\n", fileEntry->data.regionIndex, dataCompressedSize,
    // fileEntry->data.length, endTime - startTime);//
}

unsigned int ConsoleSaveFileSplit::RegionFileReference::GetCompressedSize() {
    unsigned char* dataIn = data;
    unsigned char* dataInLast = data + fileEntry->data.length;

    unsigned int outputSize = 4;  // 4 bytes required to store the uncompressed
                                  // size for faster decompression
    unsigned int runLength = 0;
    while (dataIn != dataInLast) {
        unsigned char thisByte = *dataIn++;
        if ((thisByte != 0) || (runLength == (65535 + 256))) {
            // We've got a non-zero value, or we've hit our maximum run length.
            // If there was a preceeding run of zeros, encode that nwo
            if (runLength != 0) {
                if (runLength < 256) {
                    // Runs of 1 to 255 encoded as 0 followed by one byte of run
                    // length
                    outputSize += 2;
                } else {
                    // Runs of 256 to 65791 encoded as two 0s followed by two
                    // bytes of run length - 256
                    outputSize += 4;
                }
                // Run is now processed
                runLength = 0;
            }
            // Now handle the current byte
            if (thisByte == 0) {
                runLength++;
            } else {
                // Non-zero, just copy over to output
                outputSize++;
            }
        } else {
            // It's a zero - keep counting size of the run
            runLength++;
        }
    }
    // Handle any outstanding run
    if (runLength != 0) {
        if (runLength < 256) {
            // Runs of 1 to 255 encoded as 0 followed by one byte of run length
            outputSize += 2;
        } else {
            // Runs of 256 to 65791 encoded as two 0s followed by two bytes of
            // run length - 256
            outputSize += 4;
        }
        // Run is now processed
        runLength = 0;
    }
    return outputSize;
}

// Release dataCompressed
void ConsoleSaveFileSplit::RegionFileReference::ReleaseCompressed() {
    //	Log::info("Releasing compressed data for region file from
    // 0x%.8x\n", fileEntry->data.regionIndex );
    free(dataCompressed);
    dataCompressed = nullptr;
    dataCompressedSize = 0;
}

FileEntry* ConsoleSaveFileSplit::GetRegionFileEntry(unsigned int regionIndex) {
    // Is a region file - determine if we've got it as a separate file
    auto it = regionFiles.find(regionIndex);
    if (it != regionFiles.end()) {
        // Already got it
        return it->second->fileEntry;
    }

    int index = PlatformStorage.AddSubfile(regionIndex);
    RegionFileReference* newRef = new RegionFileReference(index, regionIndex);
    regionFiles[regionIndex] = newRef;

    return newRef->fileEntry;
}

ConsoleSaveFileSplit::ConsoleSaveFileSplit(
    const std::string& fileName, void* pvSaveData /*= nullptr*/,
    unsigned int initialFileSize /*= 0*/, bool forceCleanSave /*= false*/,
    ESavePlatform plat /*= SAVE_FILE_PLATFORM_LOCAL*/) {
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

    _init(fileName, pvSaveData, fileSize, plat);

    if (bLevelGenBaseSave) {
        levelGen->deleteBaseSaveData();
    }
}

ConsoleSaveFileSplit::ConsoleSaveFileSplit(ConsoleSaveFile* sourceSave,
                                           bool alreadySmallRegions,
                                           ProgressListener* progress) {
    _init(sourceSave->getFilename(), nullptr, 0, sourceSave->getSavePlatform());

    header.setOriginalSaveVersion(sourceSave->getOriginalSaveVersion());
    header.setSaveVersion(sourceSave->getSaveVersion());

    if (alreadySmallRegions) {
        std::vector<FileEntry*>* sourceFiles =
            sourceSave->getFilesWithPrefix("");

        unsigned int bytesWritten = 0;
        for (auto it = sourceFiles->begin(); it != sourceFiles->end(); ++it) {
            FileEntry* sourceEntry = *it;
            sourceSave->setFilePointer(sourceEntry, 0,
                                       SaveFileSeekOrigin::Begin);

            FileEntry* targetEntry =
                createFile(ConsoleSavePath(sourceEntry->data.filename));

            writeFile(targetEntry, sourceSave->getWritePointer(sourceEntry),
                      sourceEntry->getFileSize(), &bytesWritten);
        }

        delete sourceFiles;
    } else {
        ConsoleSaveFileConverter::ConvertSave(sourceSave, this, progress);
    }
}

void ConsoleSaveFileSplit::_init(const std::string& fileName, void* pvSaveData,
                                 unsigned int fileSize, ESavePlatform plat) {
    m_lastTickTime = 0;

    // One time initialise of static stuff required for our storage
    if (pvHeap == nullptr) {
        // Reserve a chunk of 64MB of virtual address space for our saves, using
        // 64KB pages. We'll only be committing these as required to grow the
        // storage we need, which will the storage to grow without having to use
        // realloc.
        pvHeap = VirtualAlloc(nullptr, MAX_PAGE_COUNT * CSF_PAGE_SIZE,
                              RESERVE_ALLOCATION, PAGE_READWRITE);
    }

    pvSaveMem = pvHeap;
    m_fileName = fileName;

    // Get details of region files. From this point on we are responsible for
    // the memory that the storage manager initially allocated for them
    unsigned int regionCount = PlatformStorage.GetSubfileCount();
    for (unsigned int i = 0; i < regionCount; i++) {
        unsigned int regionIndex;
        unsigned char* regionDataCompressed;
        unsigned int regionSizeCompressed;

        PlatformStorage.GetSubfileDetails(i, (int*)&regionIndex,
                                          (void**)&regionDataCompressed,
                                          &regionSizeCompressed);

        RegionFileReference* regionFileRef = new RegionFileReference(
            i, regionIndex, regionSizeCompressed, regionDataCompressed);
        if (regionSizeCompressed > 0) {
            regionFileRef->Decompress();
        } else {
            regionFileRef->fileEntry->data.length = 0;
        }
        regionFileRef->ReleaseCompressed();
        regionFiles[regionIndex] = regionFileRef;
    }

    unsigned int heapSize = std::max(
        fileSize,
        1024u * 1024u * 2u);  // 4J Stu - Our files are going to be bigger than
                              // 2MB so allocate high to start with

    // Initially committ enough room to store headSize bytes (using
    // CSF_PAGE_SIZE pages, so rounding up here). We should only ever have one
    // save file at a time, and the pages should be decommitted in the dtor, so
    // pages committed should always be zero at this point.
    if (pagesCommitted != 0) {
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
    }

    unsigned int pagesRequired =
        (heapSize + (CSF_PAGE_SIZE - 1)) / CSF_PAGE_SIZE;

    void* pvRet = VirtualAlloc(pvHeap, pagesRequired * CSF_PAGE_SIZE,
                               COMMIT_ALLOCATION, PAGE_READWRITE);
    if (pvRet == nullptr) {
#if !defined(_CONTENT_PACKAGE)
        // Out of physical memory
        assert(0);
#endif
    }
    pagesCommitted = pagesRequired;

    if (fileSize > 0) {
        if (pvSaveData != nullptr) {
            memcpy(pvSaveMem, pvSaveData, fileSize);
        } else {
            unsigned int storageLength;
            PlatformStorage.GetSaveData(pvSaveMem, &storageLength);
            Log::info("Filesize - %d, Adjusted size - %d\n", fileSize,
                            storageLength);
            fileSize = storageLength;
        }

        int compressed = *(int*)pvSaveMem;
        if (compressed == 0) {
            unsigned int decompSize = *((int*)pvSaveMem + 1);

            // An invalid save, so clear the memory and start from scratch
            if (decompSize == 0) {
                // 4J Stu - Saves created between 2/12/2011 and 7/12/2011 will
                // have this problem
                Log::info("Invalid save data format\n");
                memset(pvSaveMem, 0, fileSize);
                // Clear the first 8 bytes that reference the header
                header.WriteHeader(pvSaveMem);
            } else {
                unsigned char* buf = new unsigned char[decompSize];

                if (Compression::getCompression()->Decompress(
                        buf, &decompSize, (unsigned char*)pvSaveMem + 8,
                        fileSize - 8) == 0) {
                    // Only ReAlloc if we need to (we might already have enough)
                    // and align to 512 byte boundaries
                    unsigned int currentHeapSize =
                        pagesCommitted * CSF_PAGE_SIZE;

                    unsigned int desiredSize = decompSize;

                    if (desiredSize > currentHeapSize) {
                        unsigned int pagesRequired =
                            (desiredSize + (CSF_PAGE_SIZE - 1)) / CSF_PAGE_SIZE;
                        void* pvRet =
                            VirtualAlloc(pvHeap, pagesRequired * CSF_PAGE_SIZE,
                                         COMMIT_ALLOCATION, PAGE_READWRITE);
                        if (pvRet == nullptr) {
                            // Out of physical memory
                            assert(0);
                        }
                        pagesCommitted = pagesRequired;
                    }

                    memcpy(pvSaveMem, buf, decompSize);
                } else {
                    // Corrupt save, although most of the terrain should
                    // actually be ok
                    Log::info("Failed to decompress save data!\n");
#if !defined(_CONTENT_PACKAGE)
                    assert(0);
#endif
                    memset(pvSaveMem, 0, fileSize);
                    // Clear the first 8 bytes that reference the header
                    header.WriteHeader(pvSaveMem);
                }

                delete[] buf;
            }
        }

        header.ReadHeader(pvSaveMem, plat);

    } else {
        // Clear the first 8 bytes that reference the header
        header.WriteHeader(pvSaveMem);
    }
}

ConsoleSaveFileSplit::~ConsoleSaveFileSplit() {
    VirtualFree(pvHeap, MAX_PAGE_COUNT * CSF_PAGE_SIZE, MEM_DECOMMIT);
    pagesCommitted = 0;
    // Make sure we don't have any thumbnail data still waiting round - we can't
    // need it now we've destroyed the save file anyway

    for (auto it = regionFiles.begin(); it != regionFiles.end(); it++) {
        delete it->second;
    }

    PlatformStorage.ResetSubfiles();
}

// Add the file to our table of internal files if not already there
// Open our actual save file ready for reading/writing, and the set the file
// pointer to the start of this file
FileEntry* ConsoleSaveFileSplit::createFile(const ConsoleSavePath& fileName) {
    LockSaveAccess();

    // Determine if the file is a region file that should be split off into its
    // own file
    unsigned int regionFileIndex;
    bool isRegionFile =
        GetNumericIdentifierFromName(fileName.getName(), &regionFileIndex);
    if (isRegionFile) {
        // First, for backwards compatibility, check if it is already in the
        // main file - will just use that if so
        if (!header.fileExists(fileName.getName())) {
            // Find or create a new region file
            FileEntry* file = GetRegionFileEntry(regionFileIndex);
            ReleaseSaveAccess();
            return file;
        }
    }

    FileEntry* file = header.AddFile(fileName.getName());
    ReleaseSaveAccess();

    return file;
}

void ConsoleSaveFileSplit::deleteFile(FileEntry* file) {
    if (file == nullptr) return;

    assert(file->isRegionFile() == false);

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

void ConsoleSaveFileSplit::setFilePointer(FileEntry* file,
                                          unsigned int distanceToMove,
                                          SaveFileSeekOrigin seekOrigin) {
    LockSaveAccess();

    if (seekOrigin == SaveFileSeekOrigin::Current) {
        file->currentFilePointer += distanceToMove;
    } else {
        if (file->isRegionFile()) {
            file->currentFilePointer = distanceToMove;
        } else {
            file->currentFilePointer = file->data.startOffset + distanceToMove;
        }

        if (seekOrigin == SaveFileSeekOrigin::End) {
            file->currentFilePointer += file->getFileSize();
        }
    }

    ReleaseSaveAccess();
}

// If this file needs to grow, move the data after along
void ConsoleSaveFileSplit::PrepareForWrite(FileEntry* file,
                                           unsigned int nNumberOfBytesToWrite) {
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

bool ConsoleSaveFileSplit::writeFile(FileEntry* file, const void* lpBuffer,
                                     unsigned int nNumberOfBytesToWrite,
                                     unsigned int* lpNumberOfBytesWritten) {
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    LockSaveAccess();

    if (file->isRegionFile()) {
        unsigned int sizeRequired =
            file->currentFilePointer + nNumberOfBytesToWrite;
        RegionFileReference* fileRef = regionFiles[file->data.regionIndex];
        if (sizeRequired > file->getFileSize()) {
            fileRef->data =
                (unsigned char*)realloc(fileRef->data, sizeRequired);
            file->data.length = sizeRequired;
        }

        memcpy(fileRef->data + file->currentFilePointer, lpBuffer,
               nNumberOfBytesToWrite);

        //		Log::info(">>>>>>>>>>>>>> writing a region file's
        // data 0x%.8x, 0x%x offset %d of %d bytes (writing %d
        // bytes)\n",file->data.regionIndex,fileRef->data,file->currentFilePointer,
        // file->getFileSize(), nNumberOfBytesToWrite);

        file->currentFilePointer += nNumberOfBytesToWrite;
        file->updateLastModifiedTime();
        fileRef->dirty = true;
    } else {
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
        // *lpNumberOfBytesWritten, file->data.filename,
        // file->currentFilePointer);

        file->updateLastModifiedTime();
    }

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileSplit::zeroFile(FileEntry* file,
                                    unsigned int nNumberOfBytesToWrite,
                                    unsigned int* lpNumberOfBytesWritten) {
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    // 4jcraft added: memset(nullptr + 0, 0, 0); was called
    // no bytes need to be written, hence there you go
    if (nNumberOfBytesToWrite == 0) {
        if (lpNumberOfBytesWritten) {
            *lpNumberOfBytesWritten = 0;
        }
        return 1;
    }

    LockSaveAccess();

    if (file->isRegionFile()) {
        unsigned int sizeRequired =
            file->currentFilePointer + nNumberOfBytesToWrite;
        RegionFileReference* fileRef = regionFiles[file->data.regionIndex];
        if (sizeRequired > file->getFileSize()) {
            fileRef->data =
                (unsigned char*)realloc(fileRef->data, sizeRequired);
            file->data.length = sizeRequired;
        }

        memset(fileRef->data + file->currentFilePointer, 0,
               nNumberOfBytesToWrite);

        //		Log::info(">>>>>>>>>>>>>> writing a region file's
        // data 0x%.8x, 0x%x offset %d of %d bytes (writing %d
        // bytes)\n",file->data.regionIndex,fileRef->data,file->currentFilePointer,
        // file->getFileSize(), nNumberOfBytesToWrite);

        file->currentFilePointer += nNumberOfBytesToWrite;
        file->updateLastModifiedTime();
        fileRef->dirty = true;
    } else {
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
        // *lpNumberOfBytesWritten, file->data.filename,
        // file->currentFilePointer);

        file->updateLastModifiedTime();
    }

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileSplit::readFile(FileEntry* file, void* lpBuffer,
                                    unsigned int nNumberOfBytesToRead,
                                    unsigned int* lpNumberOfBytesRead) {
    unsigned int actualBytesToRead;
    assert(pvSaveMem != nullptr);
    if (pvSaveMem == nullptr) {
        return false;
    }

    LockSaveAccess();

    if (file->isRegionFile()) {
        actualBytesToRead = nNumberOfBytesToRead;
        if (file->currentFilePointer + nNumberOfBytesToRead >
            file->data.length) {
            actualBytesToRead = file->data.length - file->currentFilePointer;
        }
        RegionFileReference* fileRef = regionFiles[file->data.regionIndex];
        memcpy(lpBuffer, fileRef->data + file->currentFilePointer,
               actualBytesToRead);
        *lpNumberOfBytesRead = actualBytesToRead;

        file->currentFilePointer += actualBytesToRead;
    } else {
        char* readStartOffset = (char*)pvSaveMem + file->currentFilePointer;
        // printf("Read: pvSaveMem = %0xd, currentFilePointer = %d,
        // readStartOffset = %0xd\n", pvSaveMem, file->currentFilePointer,
        // readStartOffset);

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
    }

    ReleaseSaveAccess();

    return true;
}

bool ConsoleSaveFileSplit::closeHandle(FileEntry* file) {
    LockSaveAccess();
    finalizeWrite();
    ReleaseSaveAccess();

    return true;
}

// In this method, attempt to write any dirty region files, subject to
// maintaining a maximum write output rate. Writing is prioritised by time since
// the region was last written.
void ConsoleSaveFileSplit::tick() {
    std::int64_t currentTime = System::currentTimeMillis();

    // Don't do anything if the save system is up to something...
    if (PlatformStorage.GetSaveState() != IPlatformStorage::ESaveGame_Idle) {
        return;
    }

    // ...or we shouldn't be saving...
    if (PlatformStorage.GetSaveDisabled()) {
        return;
    }

    // ... or we haven't passed the required time since last assessing what to
    // do
    if ((currentTime - m_lastTickTime) < WRITE_TICK_RATE_MS) {
        return;
    }

    LockSaveAccess();

    m_lastTickTime = currentTime;

    // Get total amount of data written over the time period we are interested
    // in averaging over. Remove any older data.
    unsigned int bytesWritten = 0;
    for (auto it = writeHistory.begin(); it != writeHistory.end();) {
        if ((currentTime - it->writeTime) >
            (WRITE_BANDWIDTH_MEASUREMENT_PERIOD_SECONDS * 1000)) {
            it = writeHistory.erase(it);
        } else {
            bytesWritten += it->writeSize;
            it++;
        }
    }

    // Compile a vector of dirty regions.
    std::vector<DirtyRegionFile> dirtyRegions;
    for (auto it = regionFiles.begin(); it != regionFiles.end(); it++) {
        DirtyRegionFile dirtyRegion;

        if (it->second->dirty) {
            dirtyRegion.fileRef = it->second->fileEntry->getRegionFileIndex();
            dirtyRegion.lastWritten = it->second->lastWritten;
            dirtyRegions.push_back(dirtyRegion);
        }
    }

    // Sort into ascending order, by lastWritten time. First elements will
    // therefore be the ones least recently saved
    std::sort(dirtyRegions.begin(), dirtyRegions.end());

    bool writeRequired = false;
    unsigned int bytesInTimePeriod = bytesWritten;
    unsigned int bytesAddedThisTick = 0;
    for (int i = 0; i < dirtyRegions.size(); i++) {
        RegionFileReference* regionRef = regionFiles[dirtyRegions[i].fileRef];
        unsigned int compressedSize = regionRef->GetCompressedSize();
        bytesInTimePeriod += compressedSize;
        bytesAddedThisTick += compressedSize;

        // Always consider at least one item for writing, even if it breaks the
        // rule on the maximum number of bytes we would like to send per tick
        if ((i > 0) && (bytesAddedThisTick > WRITE_MAX_WRITE_PER_TICK)) {
            break;
        }

        // Could we add this without breaking our bytes per second cap?
        if ((bytesInTimePeriod / WRITE_BANDWIDTH_MEASUREMENT_PERIOD_SECONDS) >
            WRITE_BANDWIDTH_BYTESPERSECOND) {
            break;
        }

        // Can add for writing
        WriteHistory writeEvent;
        writeEvent.writeSize = compressedSize;
        writeEvent.writeTime = System::currentTimeMillis();
        writeHistory.push_back(writeEvent);

        regionRef->Compress();
        //		Log::info("Tick: Writing region 0x%.8x, compressed
        // as %d bytes\n",regionRef->fileEntry->getRegionFileIndex(),
        // regionRef->dataCompressedSize);
        PlatformStorage.UpdateSubfile(regionRef->index,
                                      regionRef->dataCompressed,
                                      regionRef->dataCompressedSize);
        regionRef->dirty = false;
        regionRef->lastWritten = System::currentTimeMillis();

        writeRequired = true;
    }
#if !defined(_CONTENT_PACKAGE)
    {
        unsigned int totalDirty = 0;
        unsigned int totalDirtyBytes = 0;
        int64_t oldestDirty = currentTime;
        for (auto it = regionFiles.begin(); it != regionFiles.end(); it++) {
            if (it->second->dirty) {
                if (it->second->lastWritten < oldestDirty) {
                    oldestDirty = it->second->lastWritten;
                }
                totalDirty++;
                totalDirtyBytes += it->second->fileEntry->getFileSize();
            }
        }
    }
#endif

    if (writeRequired) {
        PlatformStorage.SaveSubfiles([this](bool bRes) {
            return SaveRegionFilesCallback(this, bRes);
        });
    }

    ReleaseSaveAccess();
}

void ConsoleSaveFileSplit::finalizeWrite() {
    LockSaveAccess();
    header.WriteHeader(pvSaveMem);
    ReleaseSaveAccess();
}

void ConsoleSaveFileSplit::MoveDataBeyond(FileEntry* file,
                                          unsigned int nNumberOfBytesToWrite) {
    unsigned int numberOfBytesRead = 0;
    unsigned int numberOfBytesWritten = 0;

    const unsigned int bufferSize = 4096;
    unsigned int amountToRead = bufferSize;
    // assert( nNumberOfBytesToWrite <= bufferSize );
    static std::uint8_t buffer1[bufferSize];
    static std::uint8_t buffer2[bufferSize];
    unsigned int buffer1Size = 0;
    unsigned int buffer2Size = 0;

    // Only ReAlloc if we need to (we might already have enough) and align to
    // 512 byte boundaries
    unsigned int currentHeapSize = pagesCommitted * CSF_PAGE_SIZE;

    unsigned int desiredSize = header.GetFileSize() + nNumberOfBytesToWrite;

    if (desiredSize > currentHeapSize) {
        unsigned int pagesRequired =
            (desiredSize + (CSF_PAGE_SIZE - 1)) / CSF_PAGE_SIZE;
        void* pvRet = VirtualAlloc(pvHeap, pagesRequired * CSF_PAGE_SIZE,
                                   COMMIT_ALLOCATION, PAGE_READWRITE);
        if (pvRet == nullptr) {
            // Out of physical memory
            assert(0);
        }
        pagesCommitted = pagesRequired;
    }

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

// Attempt to convert a filename into a numeric identifier, which we use for
// region files. File names supported are of the form:
//
// Filename				Encoded as
//
// r.x.z.mcr			00 00 xx zz
// DIM-1r.x.z.mcr		00 01 xx zz
// DIM1/r.x.z.mcr		00 02 xx zz

bool ConsoleSaveFileSplit::GetNumericIdentifierFromName(
    const std::string& fileName, unsigned int* idOut) {
    // Determine whether it is one of our region file names if the file
    // extension is ".mbr"
    if (fileName.length() < 4) return false;
    std::string extension = fileName.substr(fileName.length() - 4, 4);
    if (extension != std::string(".mcr")) return false;

    unsigned int id = 0;
    int x, z;

    const char* cstr = fileName.c_str();
    const char* body = cstr + 2;

    // If this filename starts with a "r" then assume it is of the format
    // "r.x.z.mcr" - don't do anything as default value we've set are correct
    if (cstr[0] != 'r') {
        // Must be prefixed by "DIM-1r." or "DIM1/r."
        body = cstr + 7;
        // Differentiate between these 2 options
        if (cstr[3] == '-') {
            // "DIM-1r."
            id = 0x00010000;
        } else {
            // "DIM/1r."
            id = 0x00020000;
        }
    }
    // Get x/z coords
    sscanf(body, "%d.%d.mcr", &x, &z);

    // Pack full id
    // 4jcraft added cast to unsigned
    id |= (((unsigned int)x << 8) & 0x0000ff00);
    id |= (z & 0x000000ff);

    *idOut = id;

    return true;
}

// Convert a numeric file identifier (for region files) back into a normal
// filename. See comment above.

std::string ConsoleSaveFileSplit::GetNameFromNumericIdentifier(
    unsigned int idIn) {
    std::string prefix;

    switch (idIn & 0x00ff0000) {
        case 0:
            prefix = "";
            break;
        case 1:
            prefix = "DIM-1";
            break;
        case 2:
            prefix = "DIM1/";
            break;
    }
    signed char regionX = (idIn >> 8) & 255;
    signed char regionZ = idIn & 255;
    std::string region = (prefix + std::string("r.") + toWString(regionX) +
                           "." + toWString(regionZ) + ".mcr");

    return region;
}

// Compress any dirty region files, and tell the storage manager about them so
// that it will process them when we ask it to save sub files
void ConsoleSaveFileSplit::processSubfilesForWrite() {
    for (auto it = regionFiles.begin(); it != regionFiles.end(); it++) {
        RegionFileReference* region = it->second;
        if (region->dirty) {
            region->Compress();
            PlatformStorage.UpdateSubfile(region->index, region->dataCompressed,
                                          region->dataCompressedSize);
            region->dirty = false;
            region->lastWritten = System::currentTimeMillis();
        }
    }
}

// Clean up any memory allocated for compressed data when we have finished
// writing
void ConsoleSaveFileSplit::processSubfilesAfterWrite() {
    // This is called from the PlatformStorage.Tick() which should always be on
    // the main thread
    for (auto it = regionFiles.begin(); it != regionFiles.end(); it++) {
        RegionFileReference* region = it->second;
        region->ReleaseCompressed();
    }
}

bool ConsoleSaveFileSplit::doesFileExist(ConsoleSavePath file) {
    LockSaveAccess();
    bool exists = header.fileExists(file.getName());
    ReleaseSaveAccess();

    return exists;
}

void ConsoleSaveFileSplit::Flush(bool autosave, bool updateThumbnail) {
    LockSaveAccess();

    // The storage manage might potentially be busy doing a sub-file write
    // initiated from the tick. Wait until this is totally processed.
    while (PlatformStorage.GetSaveState() != IPlatformStorage::ESaveGame_Idle) {
        Log::info("Flush wait\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    finalizeWrite();

    m_autosave = autosave;
    if (!m_autosave) processSubfilesForWrite();

    time_util::Timer timer;

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
        timer.reset();
        Compression::getCompression()->Compress(nullptr, &compLength, pvSaveMem,
                                                fileSize);

        Log::info("Check buffer size: Elapsed time %f\n",
                        static_cast<float>(timer.elapsed_seconds()));

        // We add 4 bytes to the start so that we can signal compressed data
        // And another 4 bytes to store the decompressed data size
        compLength = compLength + 8;

        // Attempt to allocate the required memory
        compData = (std::uint8_t*)PlatformStorage.AllocateSaveData(compLength);
    }

    if (compData != nullptr) {
        // Re-compress all save data before we save it to disk
        timer.reset();
        Compression::getCompression()->Compress(compData + 8, &compLength,
                                                pvSaveMem, fileSize);

        Log::info("Compress: Elapsed time %f\n",
                        static_cast<float>(timer.elapsed_seconds()));

        memset(compData, 0, 8);
        int saveVer = 0;
        memcpy(compData, &saveVer, sizeof(int));
        memcpy(compData + 4, &fileSize, sizeof(int));

        Log::info("Save data compressed from %d to %d\n", fileSize,
                        compLength);

        if (updateThumbnail) {
            std::uint8_t* pbThumbnailData = nullptr;
            unsigned int dwThumbnailDataSize = 0;

            std::uint8_t* pbDataSaveImage = nullptr;
            unsigned int dwDataSizeSaveImage = 0;

            std::uint8_t bTextMetadata[88];
            memset(bTextMetadata, 0, 88);

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

            // set the icon and save image
            PlatformStorage.SetSaveImages(pbThumbnailData, dwThumbnailDataSize,
                                          pbDataSaveImage, dwDataSizeSaveImage,
                                          bTextMetadata, iTextMetadataBytes);
            Log::info("Save thumbnail size %d\n", dwThumbnailDataSize);
        }

        int32_t saveOrCheckpointId = 0;
        bool validSave =
            PlatformStorage.GetSaveUniqueNumber(&saveOrCheckpointId);

        // save the data
        PlatformStorage.SaveSaveData([this](bool bRes) {
            return SaveSaveDataCallback(this, bRes);
        });
#if !defined(_CONTENT_PACKAGE)
        if (gameServices().debugSettingsOn()) {
            if (gameServices().getWriteSavesToFolderEnabled()) {
                DebugFlushToFile(compData, compLength + 8);
            }
        }
#endif
        ReleaseSaveAccess();
    }
}

int ConsoleSaveFileSplit::SaveSaveDataCallback(void* lpParam, bool bRes) {
    ConsoleSaveFileSplit* pClass = (ConsoleSaveFileSplit*)lpParam;

    // Don't save sub files on autosave (their always being saved anyway)
    if (!pClass->m_autosave) {
        // This is called from the PlatformStorage.Tick() which should always be
        // on the main thread
        PlatformStorage.SaveSubfiles([pClass](bool bRes) {
            return SaveRegionFilesCallback(pClass, bRes);
        });
    }
    return 0;
}

int ConsoleSaveFileSplit::SaveRegionFilesCallback(void* lpParam, bool bRes) {
    ConsoleSaveFileSplit* pClass = (ConsoleSaveFileSplit*)lpParam;

    // This is called from the PlatformStorage.Tick() which should always be on
    // the main thread
    pClass->processSubfilesAfterWrite();

    return 0;
}

#if !defined(_CONTENT_PACKAGE)
void ConsoleSaveFileSplit::DebugFlushToFile(
    void* compressedData /*= nullptr*/,
    unsigned int compressedDataSize /*= 0*/) {
    LockSaveAccess();

    finalizeWrite();

    unsigned int fileSize = header.GetFileSize();

    unsigned int numberOfBytesWritten = 0;

    File targetFileDir("Saves");

    if (!targetFileDir.exists()) targetFileDir.mkdir();

    char* fileName = new char[XCONTENT_MAX_FILENAME_LENGTH + 1];

    auto now_tp = std::chrono::system_clock::now();
    std::time_t now_tt = std::chrono::system_clock::to_time_t(now_tp);
    std::tm t{};
#if defined(_WIN32)
    gmtime_s(&t, &now_tt);
#else
    gmtime_r(&now_tt, &t);
#endif

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

unsigned int ConsoleSaveFileSplit::getSizeOnDisk() {
    return header.GetFileSize();
}

std::string ConsoleSaveFileSplit::getFilename() { return m_fileName; }

std::vector<FileEntry*>* ConsoleSaveFileSplit::getFilesWithPrefix(
    const std::string& prefix) {
    return header.getFilesWithPrefix(prefix);
}

std::vector<FileEntry*>* ConsoleSaveFileSplit::getRegionFilesByDimension(
    unsigned int dimensionIndex) {
    std::vector<FileEntry*>* files = nullptr;

    for (auto it = regionFiles.begin(); it != regionFiles.end(); ++it) {
        unsigned int entryDimension = ((it->first) >> 16) & 0xFF;

        if (entryDimension == dimensionIndex) {
            if (files == nullptr) {
                files = new std::vector<FileEntry*>();
            }

            files->push_back(it->second->fileEntry);
        }
    }

    return files;
}

int ConsoleSaveFileSplit::getSaveVersion() { return header.getSaveVersion(); }

int ConsoleSaveFileSplit::getOriginalSaveVersion() {
    return header.getOriginalSaveVersion();
}

void ConsoleSaveFileSplit::LockSaveAccess() { m_lock.lock(); }

void ConsoleSaveFileSplit::ReleaseSaveAccess() { m_lock.unlock(); }

ESavePlatform ConsoleSaveFileSplit::getSavePlatform() {
    return header.getSavePlatform();
}

bool ConsoleSaveFileSplit::isSaveEndianDifferent() {
    return header.isSaveEndianDifferent();
}

void ConsoleSaveFileSplit::setLocalPlatform() { header.setLocalPlatform(); }

void ConsoleSaveFileSplit::setPlatform(ESavePlatform plat) {
    header.setPlatform(plat);
}

std::endian ConsoleSaveFileSplit::getSaveEndian() {
    return header.getSaveEndian();
}

std::endian ConsoleSaveFileSplit::getLocalEndian() {
    return header.getLocalEndian();
}

void ConsoleSaveFileSplit::setEndian(std::endian endian) {
    header.setEndian(endian);
}

void ConsoleSaveFileSplit::ConvertRegionFile(File sourceFile) {
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
            }
        }
    }
    sourceRegionFile
        .writeAllOffsets();  // saves all the endian swapped offsets back out to
                             // the file (not all of these are written in the
                             // above processing).
}

void ConsoleSaveFileSplit::ConvertToLocalPlatform() {
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
