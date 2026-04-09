#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileConverter.h"

#include <stdio.h>
#include <wchar.h>

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "java/InputOutputStream/BufferedOutputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/util/ProgressListener.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/chunk/storage/RegionFile.h"
#include "minecraft/world/level/chunk/storage/RegionFileCache.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileInputStream.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/DirectoryLevelStorage.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "nbt/CompoundTag.h"
#include "nbt/NbtIo.h"
#include "strings.h"

void ConsoleSaveFileConverter::ProcessSimpleFile(ConsoleSaveFile* sourceSave,
                                                 FileEntry* sourceFileEntry,
                                                 ConsoleSaveFile* targetSave,
                                                 FileEntry* targetFileEntry) {
    unsigned int numberOfBytesRead = 0;
    unsigned int numberOfBytesWritten = 0;

    std::uint8_t* data = new std::uint8_t[sourceFileEntry->getFileSize()];

    // Read from source
    sourceSave->readFile(sourceFileEntry, data, sourceFileEntry->getFileSize(),
                         &numberOfBytesRead);

    // Write back to target
    targetSave->writeFile(targetFileEntry, data, numberOfBytesRead,
                          &numberOfBytesWritten);

    delete[] data;
}

void ConsoleSaveFileConverter::ProcessStandardRegionFile(
    ConsoleSaveFile* sourceSave, File sourceFile, ConsoleSaveFile* targetSave,
    File targetFile) {
    unsigned int numberOfBytesWritten = 0;
    unsigned int numberOfBytesRead = 0;

    RegionFile sourceRegionFile(sourceSave, &sourceFile);
    RegionFile targetRegionFile(targetSave, &targetFile);

    for (unsigned int x = 0; x < 32; ++x) {
        for (unsigned int z = 0; z < 32; ++z) {
            DataInputStream* dis =
                sourceRegionFile.getChunkDataInputStream(x, z);

            if (dis) {
                int read = dis->read();
                DataOutputStream* dos =
                    targetRegionFile.getChunkDataOutputStream(x, z);
                while (read != -1) {
                    dos->write(read & 0xff);

                    read = dis->read();
                }
                dos->close();
                dos->deleteChildStream();
                delete dos;
            }

            delete dis;
        }
    }
}

void ConsoleSaveFileConverter::ConvertSave(ConsoleSaveFile* sourceSave,
                                           ConsoleSaveFile* targetSave,
                                           ProgressListener* progress) {
    // Process level.dat
    ConsoleSavePath ldatPath(std::string("level.dat"));
    FileEntry* sourceLdatFe = sourceSave->createFile(ldatPath);
    FileEntry* targetLdatFe = targetSave->createFile(ldatPath);
    printf("Processing level.dat\n");
    ProcessSimpleFile(sourceSave, sourceLdatFe, targetSave, targetLdatFe);

    // Process game rules
    {
        ConsoleSavePath gameRulesPath(GAME_RULE_SAVENAME);
        if (sourceSave->doesFileExist(gameRulesPath)) {
            FileEntry* sourceFe = sourceSave->createFile(gameRulesPath);
            FileEntry* targetFe = targetSave->createFile(gameRulesPath);
            printf("Processing game rules\n");
            ProcessSimpleFile(sourceSave, sourceFe, targetSave, targetFe);
        }
    }

    // MGH added - find any player data files and copy them across
    std::vector<FileEntry*>* playerFiles =
        sourceSave->getFilesWithPrefix(DirectoryLevelStorage::getPlayerDir());

    if (playerFiles != nullptr) {
        for (int fileIdx = 0; fileIdx < playerFiles->size(); fileIdx++) {
            ConsoleSavePath sourcePlayerDatPath(
                playerFiles->at(fileIdx)->data.filename);
            ConsoleSavePath targetPlayerDatPath(
                playerFiles->at(fileIdx)->data.filename);
            {
                FileEntry* sourceFe =
                    sourceSave->createFile(sourcePlayerDatPath);
                FileEntry* targetFe =
                    targetSave->createFile(targetPlayerDatPath);
                printf("Processing player dat file %s\n",
                       playerFiles->at(fileIdx)->data.filename);
                ProcessSimpleFile(sourceSave, sourceFe, targetSave, targetFe);

                targetFe->data.lastModifiedTime =
                    sourceFe->data.lastModifiedTime;
            }
        }
        delete playerFiles;
    }

#if defined(SPLIT_SAVES)
    int xzSize = LEVEL_LEGACY_WIDTH;
    int hellScale = HELL_LEVEL_LEGACY_SCALE;
    if (sourceSave->doesFileExist(ldatPath)) {
        ConsoleSaveFileInputStream fis =
            ConsoleSaveFileInputStream(sourceSave, ldatPath);
        CompoundTag* root = NbtIo::readCompressed(&fis);
        CompoundTag* tag = root->getCompound("Data");
        LevelData ret(tag);

        xzSize = ret.getXZSize();
        hellScale = ret.getHellScale();

        delete root;
    }

    RegionFileCache sourceCache;
    RegionFileCache targetCache;

    if (progress) {
        progress->progressStage(IDS_SAVETRANSFER_STAGE_CONVERTING);
    }

    // Overworld
    {
        printf("Processing the overworld\n");
        int halfXZSize = xzSize / 2;

        int progressTarget = (xzSize) * (xzSize);
        int currentProgress = 0;
        if (progress)
            progress->progressStagePercentage((currentProgress * 100) /
                                              progressTarget);

        for (int x = -halfXZSize; x < halfXZSize; ++x) {
            for (int z = -halfXZSize; z < halfXZSize; ++z) {
                // printf("Processing overworld chunk %d,%d\n",x,z);
                DataInputStream* dis =
                    sourceCache._getChunkDataInputStream(sourceSave, "", x, z);

                if (dis) {
                    int read = dis->read();
                    DataOutputStream* dos =
                        targetCache._getChunkDataOutputStream(targetSave, "", x,
                                                              z);
                    BufferedOutputStream bos(dos, 1024 * 1024);
                    while (read != -1) {
                        bos.write(read & 0xff);

                        read = dis->read();
                    }
                    bos.flush();
                    dos->close();
                    dos->deleteChildStream();
                    delete dos;
                }

                delete dis;

                ++currentProgress;
                if (progress)
                    progress->progressStagePercentage((currentProgress * 100) /
                                                      progressTarget);
            }
        }
    }

    // Nether
    {
        printf("Processing the nether\n");
        int hellSize = xzSize / hellScale;
        int halfXZSize = hellSize / 2;

        int progressTarget = (hellSize) * (hellSize);
        int currentProgress = 0;
        if (progress)
            progress->progressStagePercentage((currentProgress * 100) /
                                              progressTarget);

        for (int x = -halfXZSize; x < halfXZSize; ++x) {
            for (int z = -halfXZSize; z < halfXZSize; ++z) {
                // printf("Processing nether chunk %d,%d\n",x,z);
                DataInputStream* dis = sourceCache._getChunkDataInputStream(
                    sourceSave, "DIM-1", x, z);

                if (dis) {
                    int read = dis->read();
                    DataOutputStream* dos =
                        targetCache._getChunkDataOutputStream(targetSave,
                                                              "DIM-1", x, z);
                    BufferedOutputStream bos(dos, 1024 * 1024);
                    while (read != -1) {
                        bos.write(read & 0xff);

                        read = dis->read();
                    }
                    bos.flush();
                    dos->close();
                    dos->deleteChildStream();
                    delete dos;
                }

                delete dis;

                ++currentProgress;
                if (progress)
                    progress->progressStagePercentage((currentProgress * 100) /
                                                      progressTarget);
            }
        }
    }

    // End
    {
        printf("Processing the end\n");
        int halfXZSize = END_LEVEL_MAX_WIDTH / 2;

        int progressTarget = (END_LEVEL_MAX_WIDTH) * (END_LEVEL_MAX_WIDTH);
        int currentProgress = 0;
        if (progress)
            progress->progressStagePercentage((currentProgress * 100) /
                                              progressTarget);

        for (int x = -halfXZSize; x < halfXZSize; ++x) {
            for (int z = -halfXZSize; z < halfXZSize; ++z) {
                // printf("Processing end chunk %d,%d\n",x,z);
                DataInputStream* dis = sourceCache._getChunkDataInputStream(
                    sourceSave, "DIM1/", x, z);

                if (dis) {
                    int read = dis->read();
                    DataOutputStream* dos =
                        targetCache._getChunkDataOutputStream(targetSave,
                                                              "DIM1/", x, z);
                    BufferedOutputStream bos(dos, 1024 * 1024);
                    while (read != -1) {
                        bos.write(read & 0xff);

                        read = dis->read();
                    }
                    bos.flush();
                    dos->close();
                    dos->deleteChildStream();
                    delete dos;
                }

                delete dis;

                ++currentProgress;
                if (progress)
                    progress->progressStagePercentage((currentProgress * 100) /
                                                      progressTarget);
            }
        }
    }

#else
    // 4J Stu - Old version that just changes the compression of chunks, not
    // usable for XboxOne style split saves or compressed tile formats Process
    // region files
    std::vector<FileEntry*>* allFilesInSave =
        sourceSave->getFilesWithPrefix(std::string(""));
    for (auto it = allFilesInSave->begin(); it < allFilesInSave->end(); ++it) {
        FileEntry* fe = *it;
        if (fe != sourceLdatFe) {
            std::string fName(fe->data.filename);
            std::string suffix(".mcr");
            if (fName.compare(fName.length() - suffix.length(), suffix.length(),
                              suffix) == 0) {
#if !defined(_CONTENT_PACKAGE)
                printf("Processing a region file: %s\n", fe->data.filename);
#endif
                ProcessStandardRegionFile(sourceSave, File(fe->data.filename),
                                          targetSave, File(fe->data.filename));
            } else {
#if !defined(_CONTENT_PACKAGE)
                printf("%s is not a region file, ignoring\n",
                       fe->data.filename);
#endif
            }
        }
    }
#endif
}
