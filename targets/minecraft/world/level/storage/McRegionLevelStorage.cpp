#include "McRegionLevelStorage.h"

#include <stdint.h>

#include <format>
#include <vector>

#include "LevelData.h"
#include "java/File.h"
#include "minecraft/IGameServices.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/chunk/storage/McRegionChunkStorage.h"
#include "minecraft/world/level/chunk/storage/RegionFileCache.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/dimension/HellDimension.h"
#include "minecraft/world/level/dimension/TheEndDimension.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/DirectoryLevelStorage.h"
#include "minecraft/world/level/storage/LevelStorage.h"

McRegionLevelStorage::McRegionLevelStorage(ConsoleSaveFile* saveFile, File dir,
                                           const std::string& levelName,
                                           bool createPlayerDir)
    : DirectoryLevelStorage(saveFile, dir, levelName, createPlayerDir) {
    RegionFileCache::clear();
}

McRegionLevelStorage::~McRegionLevelStorage() {
    // Make sure cache is clear, as the DirectoryLevelStorage destructor is
    // going to be deleting the underlying ConsoleSaveFile reference so we don't
    // want the RegionFileCache to still be referencing it either
    RegionFileCache::clear();
}

ChunkStorage* McRegionLevelStorage::createChunkStorage(Dimension* dimension) {
    // File folder = getFolder();

    if (dynamic_cast<HellDimension*>(dimension) != nullptr) {
        if (gameServices().getResetNether()) {
#ifdef SPLIT_SAVES
            std::vector<FileEntry*>* netherFiles =
                m_saveFile->getRegionFilesByDimension(1);
            if (netherFiles != nullptr) {
                uint32_t bytesWritten = 0;
                for (auto it = netherFiles->begin(); it != netherFiles->end();
                     ++it) {
                    m_saveFile->zeroFile(*it, (*it)->getFileSize(),
                                         &bytesWritten);
                }
                delete netherFiles;
            }
#else
            std::vector<FileEntry*>* netherFiles =
                m_saveFile->getFilesWithPrefix(LevelStorage::NETHER_FOLDER);
            if (netherFiles != nullptr) {
                for (auto it = netherFiles->begin(); it != netherFiles->end();
                     ++it) {
                    m_saveFile->deleteFile(*it);
                }
                delete netherFiles;
            }
#endif
            resetNetherPlayerPositions();
        }

        return new McRegionChunkStorage(m_saveFile,
                                        LevelStorage::NETHER_FOLDER);
    }

    if (dynamic_cast<TheEndDimension*>(dimension)) {
        // File dir2 = new File(folder, LevelStorage.ENDER_FOLDER);
        // dir2.mkdirs();
        // return new ThreadedMcRegionChunkStorage(dir2);

        // 4J-PB - save version 0 at this point means it's a create new world
        int iSaveVersion = m_saveFile->getSaveVersion();

        if ((iSaveVersion != 0) && (iSaveVersion < SAVE_FILE_VERSION_NEW_END)) {
            // For versions before TU9 (TU7 and 8) we generate a part of The
            // End, but we want to scrap it if it exists so that it is replaced
            // with the TU9+ version
            Log::info(
                "Loaded save version number is: %d, required to keep The End "
                "is: %d\n",
                m_saveFile->getSaveVersion(), SAVE_FILE_VERSION_NEW_END);

            std::vector<FileEntry*>* endFiles =
                m_saveFile->getFilesWithPrefix(LevelStorage::ENDER_FOLDER);

            // 4J-PB - There will be no End in early saves
            if (endFiles != nullptr) {
                for (auto it = endFiles->begin(); it != endFiles->end(); ++it) {
                    m_saveFile->deleteFile(*it);
                }
                delete endFiles;
            }
        }
        return new McRegionChunkStorage(m_saveFile, LevelStorage::ENDER_FOLDER);
    }

    return new McRegionChunkStorage(m_saveFile, "");
}

void McRegionLevelStorage::saveLevelData(
    LevelData* levelData, std::vector<std::shared_ptr<Player> >* players) {
    levelData->setVersion(MCREGION_VERSION_ID);
    DirectoryLevelStorage::saveLevelData(levelData, players);
}

void McRegionLevelStorage::closeAll() { RegionFileCache::clear(); }