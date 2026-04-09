#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "util/Definitions.h"

class ConsoleSaveFileOriginal : public ConsoleSaveFile {
private:
    FileHeader header;

    std::string m_fileName;

    // Backing store for the in-memory save image. The buffer is sized to
    // MAX_SAVE_SIZE up front; on Linux/macOS the kernel only physically backs
    // pages that are actually touched, so this gives the same demand-paging
    // behaviour the legacy VirtualAlloc reserve/commit pattern relied on,
    // without any OS-specific calls.
#if defined(_LARGE_WORLDS)
    static constexpr std::size_t MAX_SAVE_SIZE =
        2u * 1024u * 1024u * 1024u;  // 2GB
#else
    static constexpr std::size_t MAX_SAVE_SIZE = 64u * 1024u * 1024u;  // 64MB
#endif
    std::vector<std::uint8_t> saveBuffer;
    void* pvSaveMem = saveBuffer.data();

    std::recursive_mutex m_lock;

    void PrepareForWrite(FileEntry* file, unsigned int nNumberOfBytesToWrite);
    void MoveDataBeyond(FileEntry* file, unsigned int nNumberOfBytesToWrite);

public:
#if defined(_WINDOWS64)
    static int SaveSaveDataCallback(void* lpParam, bool bRes);
#endif
    ConsoleSaveFileOriginal(const std::string& fileName,
                            void* pvSaveData = nullptr,
                            unsigned int fileSize = 0,
                            bool forceCleanSave = false,
                            ESavePlatform plat = SAVE_FILE_PLATFORM_LOCAL);
    virtual ~ConsoleSaveFileOriginal();

    // 4J Stu - Initial implementation is intended to have a similar interface
    // to the standard Xbox file access functions

    virtual FileEntry* createFile(const ConsoleSavePath& fileName);
    virtual void deleteFile(FileEntry* file);

    virtual void setFilePointer(FileEntry* file, unsigned int distanceToMove,
                                SaveFileSeekOrigin seekOrigin);
    virtual bool writeFile(FileEntry* file, const void* lpBuffer,
                           unsigned int nNumberOfBytesToWrite,
                           unsigned int* lpNumberOfBytesWritten);
    virtual bool zeroFile(FileEntry* file, unsigned int nNumberOfBytesToWrite,
                          unsigned int* lpNumberOfBytesWritten);
    virtual bool readFile(FileEntry* file, void* lpBuffer,
                          unsigned int nNumberOfBytesToRead,
                          unsigned int* lpNumberOfBytesRead);
    virtual bool closeHandle(FileEntry* file);

    virtual void finalizeWrite();

    virtual bool doesFileExist(ConsoleSavePath file);

    virtual void Flush(bool autosave, bool updateThumbnail = true);

#if !defined(_CONTENT_PACKAGE)
    virtual void DebugFlushToFile(void* compressedData = nullptr,
                                  unsigned int compressedDataSize = 0);
#endif
    virtual unsigned int getSizeOnDisk();

    virtual std::string getFilename();

    virtual std::vector<FileEntry*>* getFilesWithPrefix(
        const std::string& prefix);
    virtual std::vector<FileEntry*>* getRegionFilesByDimension(
        unsigned int dimensionIndex);

    virtual int getSaveVersion();
    virtual int getOriginalSaveVersion();

    virtual void LockSaveAccess();
    virtual void ReleaseSaveAccess();

    virtual ESavePlatform getSavePlatform();
    virtual bool isSaveEndianDifferent();
    virtual void setLocalPlatform();
    virtual void setPlatform(ESavePlatform plat);
    virtual std::endian getSaveEndian();
    virtual std::endian getLocalEndian();
    virtual void setEndian(std::endian endian);
    virtual bool isLocalEndianDifferent(ESavePlatform plat);

    virtual void ConvertRegionFile(File sourceFile);
    virtual void ConvertToLocalPlatform();

protected:
    virtual void* getWritePointer(FileEntry* file);
};
