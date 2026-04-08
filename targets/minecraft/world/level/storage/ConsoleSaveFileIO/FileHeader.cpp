#include "minecraft/util/Log.h"

// #define _DEBUG_FILE_HEADER

#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"

#include <assert.h>
#include <wchar.h>

#include <algorithm>
#include <compare>
#include <string>
#include <vector>

#include "app/linux/LinuxGame.h"
#include "util/Definitions.h"
#include "java/System.h"

extern LinuxGame app;

FileHeader::FileHeader() {
    lastFile = nullptr;
    m_saveVersion = 0;

    // New saves should have an original version set to the latest version. This
    // will be overridden when we load a save
    m_originalSaveVersion = SAVE_FILE_VERSION_NUMBER;
    m_savePlatform = SAVE_FILE_PLATFORM_LOCAL;
    m_saveEndian = m_localEndian;
}

FileHeader::~FileHeader() {
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        delete fileTable[i];
    }
}

FileEntry* FileHeader::AddFile(const std::string& name,
                               unsigned int length /* = 0 */) {
    assert(name.length() < 64);

    char filename[64];
    memset(&filename, 0, sizeof(char) * 64);
    memcpy(&filename, name.c_str(),
           std::min(sizeof(char) * 64, sizeof(char) * name.length()));

    // Would a map be more efficient? Our file tables probably won't be very big
    // so better to avoid hashing all the time? Does the file exist?
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        if (strcmp(fileTable[i]->data.filename, filename) == 0) {
            // If so, return it
            return fileTable[i];
        }
    }

    // Else, add it to our file table
    fileTable.push_back(new FileEntry(filename, length, GetStartOfNextData()));
    lastFile = fileTable[fileTable.size() - 1];
    return lastFile;
}

void FileHeader::RemoveFile(FileEntry* file) {
    if (file == nullptr) return;

    AdjustStartOffsets(file, file->getFileSize(), true);

    auto it = find(fileTable.begin(), fileTable.end(), file);

    if (it < fileTable.end()) {
        fileTable.erase(it);
    }

#if !defined(_CONTENT_PACKAGE)
    printf("Removed file %s\n", file->data.filename);
#endif

    delete file;
}

void FileHeader::WriteHeader(void* saveMem) {
    unsigned int headerOffset = GetStartOfNextData();

    // 4J Changed for save version 2 to be the number of files rather than the
    // size in bytes
    unsigned int headerSize = (int)(fileTable.size());

    // uint32_t numberOfBytesWritten = 0;

    // Write the offset of the header
    // assert(numberOfBytesWritten == 4);
    int* begin = (int*)saveMem;
    *begin = headerOffset;

    // Write the size of the header
    // assert(numberOfBytesWritten == 4);
    *(begin + 1) = headerSize;

    short* versions = (short*)(begin + 2);
    // Write the original version number
    *versions = m_originalSaveVersion;

    // Write the version number
    short versionNumber = SAVE_FILE_VERSION_NUMBER;
    // assert(numberOfBytesWritten == 4);
    //*(begin + 2) = versionNumber;
    *(versions + 1) = versionNumber;

#if defined(_DEBUG_FILE_HEADER)
    Log::info(
        "Write save file with original version: %d, and current version %d\n",
        m_originalSaveVersion, versionNumber);
#endif

    char* headerPosition = (char*)saveMem + headerOffset;

#if defined(_DEBUG_FILE_HEADER)
    Log::info("\n\nWrite file Header: Offset = %d, Size = %d\n",
                    headerOffset, headerSize);
#endif

    // Write the header
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        // printf("File: %s, Start = %d, Length = %d, End = %d\n",
        // fileTable[i]->data.filename, fileTable[i]->data.startOffset,
        // fileTable[i]->data.size(), fileTable[i]->data.startOffset +
        // fileTable[i]->data.size());
        memcpy((void*)headerPosition, &fileTable[i]->data,
               sizeof(FileEntrySaveData));
        // assert(numberOfBytesWritten == sizeof(FileEntrySaveData));
        headerPosition += sizeof(FileEntrySaveData);
    }
}

void FileHeader::ReadHeader(
    void* saveMem, ESavePlatform plat /*= SAVE_FILE_PLATFORM_LOCAL */) {
    unsigned int headerOffset;
    unsigned int headerSize;

    m_savePlatform = plat;

    switch (m_savePlatform) {
        case SAVE_FILE_PLATFORM_X360:
        case SAVE_FILE_PLATFORM_PS3:
            m_saveEndian = std::endian::big;
            break;
        case SAVE_FILE_PLATFORM_XBONE:
        case SAVE_FILE_PLATFORM_WIN64:
        case SAVE_FILE_PLATFORM_PS4:
        case SAVE_FILE_PLATFORM_PSVITA:
            m_saveEndian = std::endian::little;
            break;
        default:
            assert(0);
            m_savePlatform = SAVE_FILE_PLATFORM_LOCAL;
            m_saveEndian = m_localEndian;
            break;
    }

    // Read the offset of the header
    // assert(numberOfBytesRead == 4);
    int* begin = (int*)saveMem;
    headerOffset = *begin;
    if (isSaveEndianDifferent()) System::ReverseULONG(&headerOffset);

    // Read the size of the header
    // assert(numberOfBytesRead == 4);
    headerSize = *(begin + 1);
    if (isSaveEndianDifferent()) System::ReverseULONG(&headerSize);

    short* versions = (short*)(begin + 2);
    // Read the original save version number
    m_originalSaveVersion = *(versions);
    if (isSaveEndianDifferent()) System::ReverseSHORT(&m_originalSaveVersion);

    // Read the save version number
    // m_saveVersion = *(begin + 2);
    m_saveVersion = *(versions + 1);
    if (isSaveEndianDifferent()) System::ReverseSHORT(&m_saveVersion);

#if defined(_DEBUG_FILE_HEADER)
    Log::info(
        "Read save file with orignal version: %d, and current version %d\n",
        m_originalSaveVersion, m_saveVersion);
    Log::info("\n\nRead file Header: Offset = %d, Size = %d\n",
                    headerOffset, headerSize);
#endif

    char* headerPosition = (char*)saveMem + headerOffset;

    switch (m_saveVersion) {
        // case SAVE_FILE_VERSION_NUMBER:
        // case 8: // 4J Stu - SAVE_FILE_VERSION_NUMBER 2,3,4,5,6,7,8 are the
        // same, but: 							: Bumped
        // it to 3 in TU5 to force older builds (ie 0062) to
        // generate a new world when trying to load new saves
        // : Bumped it to 4 in TU9 to delete versions of The End that were
        // generated in builds prior to TU9
        // : Bumped it to 5 in TU9 to update the map data that was only using 1
        // bit to determine dimension
        // : Bumped it to 6 for PS3 v1 to update map data mappings to use larger
        // PlayerUID 							: Bumped
        // it to 7 for Durango v1 to update map data mappings to use string
        // based PlayerUID
        // : Bumped it to 8 for Durango v1 when to save the chunks in a
        // different compressed format
        case SAVE_FILE_VERSION_COMPRESSED_CHUNK_STORAGE:
        case SAVE_FILE_VERSION_DURANGO_CHANGE_MAP_DATA_MAPPING_SIZE:
        case SAVE_FILE_VERSION_CHANGE_MAP_DATA_MAPPING_SIZE:
        case SAVE_FILE_VERSION_MOVED_STRONGHOLD:
        case SAVE_FILE_VERSION_NEW_END:
        case SAVE_FILE_VERSION_POST_LAUNCH:
        case SAVE_FILE_VERSION_LAUNCH: {
            // Changes for save file version 2:
            // headerSize is now a count of elements rather than a count of
            // bytes The FileEntrySaveData struct has a lastModifiedTime member

            // Read the header
            FileEntrySaveData* fesdHeaderPosition =
                (FileEntrySaveData*)headerPosition;
            for (unsigned int i = 0; i < headerSize; ++i) {
                FileEntry* entry = new FileEntry();
                // assert(numberOfBytesRead == sizeof(FileEntrySaveData));

                memcpy(&entry->data, fesdHeaderPosition,
                       sizeof(FileEntrySaveData));

                if (isSaveEndianDifferent()) {
                    // Reverse bytes
                    // System::ReverseWCHARA(entry->data.filename,64);
                    System::ReverseULONG(&entry->data.length);
                    System::ReverseULONG(&entry->data.startOffset);
                    System::ReverseULONGLONG(&entry->data.lastModifiedTime);
                }

                entry->currentFilePointer = entry->data.startOffset;
                lastFile = entry;
                fileTable.push_back(entry);
#if defined(_DEBUG_FILE_HEADER)
                Log::info(
                    "File: %s, Start = %d, Length = %d, End = %d, Timestamp = "
                    "%lld\n",
                    entry->data.filename, entry->data.startOffset,
                    entry->data.length,
                    entry->data.startOffset + entry->data.length,
                    entry->data.lastModifiedTime);
#endif

                fesdHeaderPosition++;
            }
        } break;

        // Legacy save versions, with updated code to convert the
        // FileEntrySaveData to the latest version 4J Stu - At time of writing,
        // the tutorial save is V1 so need to keep this for compatibility
        case SAVE_FILE_VERSION_PRE_LAUNCH: {
            // Read the header
            // We can then make headerPosition a FileEntrySaveData pointer and
            // just increment by one up to the number
            unsigned int i = 0;
            while (i < headerSize) {
                FileEntry* entry = new FileEntry();
                // assert(numberOfBytesRead == sizeof(FileEntrySaveData));

                memcpy(&entry->data, headerPosition,
                       sizeof(FileEntrySaveDataV1));

                entry->currentFilePointer = entry->data.startOffset;
                lastFile = entry;
                fileTable.push_back(entry);
#if defined(_DEBUG_FILE_HEADER)
                Log::info(
                    "File: %s, Start = %d, Length = %d, End = %d\n",
                    entry->data.filename, entry->data.startOffset,
                    entry->data.length,
                    entry->data.startOffset + entry->data.length);
#endif

                i += sizeof(FileEntrySaveDataV1);
                headerPosition += sizeof(FileEntrySaveDataV1);
            }
        } break;
        default:
#if !defined(_CONTENT_PACKAGE)
            Log::info("**********  Invalid save version %d\n",
                            m_saveVersion);
            assert(0);
#endif
            break;
    }
}

unsigned int FileHeader::GetStartOfNextData() {
    // The first 4 bytes is the location of the header (the header itself is at
    // the end of the file) Then 4 bytes for the size of the header Then 2 bytes
    // for the version number at which this save was first generated Then 2
    // bytes for the version number that the save should now be at
    unsigned int totalBytesSoFar = SAVE_FILE_HEADER_SIZE;
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        if (fileTable[i]->getFileSize() > 0)
            totalBytesSoFar += fileTable[i]->getFileSize();
    }
    return totalBytesSoFar;
}

unsigned int FileHeader::GetFileSize() {
    return GetStartOfNextData() +
           (sizeof(FileEntrySaveData) * (unsigned int)fileTable.size());
}

void FileHeader::AdjustStartOffsets(FileEntry* file,
                                    unsigned int nNumberOfBytesToWrite,
                                    bool subtract /*= false*/) {
    bool found = false;
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        if (found == true) {
            if (subtract) {
                fileTable[i]->data.startOffset -= nNumberOfBytesToWrite;
                fileTable[i]->currentFilePointer -= nNumberOfBytesToWrite;
            } else {
                fileTable[i]->data.startOffset += nNumberOfBytesToWrite;
                fileTable[i]->currentFilePointer += nNumberOfBytesToWrite;
            }
        } else if (fileTable[i] == file) {
            found = true;
        }
    }
}

bool FileHeader::fileExists(const std::string& name) {
    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        if (strcmp(fileTable[i]->data.filename, name.c_str()) == 0) {
            // If so, return it
            return true;
        }
    }
    return false;
}

std::vector<FileEntry*>* FileHeader::getFilesWithPrefix(
    const std::string& prefix) {
    std::vector<FileEntry*>* files = nullptr;

    for (unsigned int i = 0; i < fileTable.size(); ++i) {
        if (strncmp(fileTable[i]->data.filename, prefix.c_str(),
                    prefix.size()) == 0) {
            if (files == nullptr) {
                files = new std::vector<FileEntry*>();
            }

            files->push_back(fileTable[i]);
        }
    }

    return files;
}

std::endian FileHeader::getEndian(ESavePlatform plat) {
    std::endian platEndian;
    switch (plat) {
        case SAVE_FILE_PLATFORM_X360:
        case SAVE_FILE_PLATFORM_PS3:
            return std::endian::big;
            break;

        case SAVE_FILE_PLATFORM_NONE:
        case SAVE_FILE_PLATFORM_XBONE:
        case SAVE_FILE_PLATFORM_PS4:
        case SAVE_FILE_PLATFORM_PSVITA:
        case SAVE_FILE_PLATFORM_WIN64:
            return std::endian::little;
            break;
        default:
            assert(0);
            break;
    }
    return std::endian::little;
}