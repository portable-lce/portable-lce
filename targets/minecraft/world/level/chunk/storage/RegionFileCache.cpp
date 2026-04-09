#include "RegionFileCache.h"

#include <utility>

#include "java/File.h"
#include "minecraft/world/level/chunk/storage/RegionFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "util/StringHelpers.h"

class DataInputStream;
class DataOutputStream;

RegionFileCache RegionFileCache::s_defaultCache;

bool RegionFileCache::useSplitSaves(ESavePlatform platform) {
    switch (platform) {
        case SAVE_FILE_PLATFORM_XBONE:
        case SAVE_FILE_PLATFORM_PS4:
            return true;
        default:
            return false;
    };
}

RegionFile* RegionFileCache::_getRegionFile(
    ConsoleSaveFile* saveFile, const std::string& prefix, int chunkX,
    int chunkZ)  // 4J - TODO was synchronized
{
    // 4J Jev - changed back to use of the File class.
    // char file[MAX_PATH_SIZE];
    // sprintf(file,"%s\\region\\r.%d.%d.mcr",basePath,chunkX >> 5,chunkZ >> 5);

    // File regionDir(basePath, "region");

    // File file(regionDir, string("r.") + toWString(chunkX>>5) + "." +
    // toWString(chunkZ>>5) + ".mcr" );
    File file;
    if (useSplitSaves(saveFile->getSavePlatform())) {
        file = File(prefix + std::string("r.") + toWString(chunkX >> 4) + "." +
                    toWString(chunkZ >> 4) + ".mcr");
    } else {
        file = File(prefix + std::string("r.") + toWString(chunkX >> 5) + "." +
                    toWString(chunkZ >> 5) + ".mcr");
    }

    RegionFile* ref = nullptr;
    auto it = cache.find(file);
    if (it != cache.end()) ref = it->second;

    // 4J Jev, put back in.
    if (ref != nullptr) {
        return ref;
    }

    // 4J Stu - Remove for new save files
    /*
if (!regionDir.exists())
    {
    regionDir.mkdirs();
}
    */
    if (cache.size() >= MAX_CACHE_SIZE) {
        _clear();
    }

    RegionFile* reg = new RegionFile(saveFile, &file);
    cache[file] = reg;  // 4J - this was originally a softReferenc
    return reg;
}

void RegionFileCache::_clear()  // 4J - TODO was synchronized
{
    auto itEnd = cache.end();
    for (auto it = cache.begin(); it != itEnd; it++) {
        // 4J - removed try/catch
        //        try {
        RegionFile* regionFile = it->second;
        if (regionFile != nullptr) {
            regionFile->close();
        }
        delete regionFile;
        //        } catch (IOException e) {
        //            e.printStackTrace();
        //        }
    }
    cache.clear();
}

int RegionFileCache::_getSizeDelta(ConsoleSaveFile* saveFile,
                                   const std::string& prefix, int chunkX,
                                   int chunkZ) {
    RegionFile* r = _getRegionFile(saveFile, prefix, chunkX, chunkZ);
    return r->getSizeDelta();
}

DataInputStream* RegionFileCache::_getChunkDataInputStream(
    ConsoleSaveFile* saveFile, const std::string& prefix, int chunkX,
    int chunkZ) {
    RegionFile* r = _getRegionFile(saveFile, prefix, chunkX, chunkZ);
    if (useSplitSaves(saveFile->getSavePlatform())) {
        return r->getChunkDataInputStream(chunkX & 15, chunkZ & 15);
    } else {
        return r->getChunkDataInputStream(chunkX & 31, chunkZ & 31);
    }
}

DataOutputStream* RegionFileCache::_getChunkDataOutputStream(
    ConsoleSaveFile* saveFile, const std::string& prefix, int chunkX,
    int chunkZ) {
    RegionFile* r = _getRegionFile(saveFile, prefix, chunkX, chunkZ);
    if (useSplitSaves(saveFile->getSavePlatform())) {
        return r->getChunkDataOutputStream(chunkX & 15, chunkZ & 15);
    } else {
        return r->getChunkDataOutputStream(chunkX & 31, chunkZ & 31);
    }
}

RegionFileCache::~RegionFileCache() { _clear(); }
