#include "MinecraftServer.h"

#include <assert.h>
#include <wchar.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <utility>

#include "ConsoleInput.h"
#include "DispenserBootstrap.h"
#include "PlayerList.h"
#include "Settings.h"
#include "java/Class.h"
#include "java/File.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/InputOutputStream/FileOutputStream.h"
#include "java/Random.h"
#include "java/System.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/Pos.h"
#include "minecraft/client/Options.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/commands/Command.h"
#include "minecraft/network/INetworkService.h"
#include "minecraft/network/packet/GameEventPacket.h"
#include "minecraft/network/packet/ServerSettingsChangedPacket.h"
#include "minecraft/network/packet/SetTimePacket.h"
#include "minecraft/network/packet/UpdateProgressPacket.h"
#include "platform/network/network.h"
#include "minecraft/server/level/DerivedServerLevel.h"
#include "minecraft/server/level/EntityTracker.h"
#include "minecraft/server/level/ServerChunkCache.h"
#include "minecraft/server/level/ServerLevel.h"
#include "minecraft/server/network/ServerConnection.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityIO.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/GameRules.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "minecraft/world/level/LevelSettings.h"
#include "minecraft/world/level/LevelType.h"
#include "minecraft/world/level/chunk/ChunkSource.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFile.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSavePath.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/FileHeader.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/storage/LevelStorage.h"
#include "minecraft/world/level/storage/McRegionLevelStorage.h"
#include "minecraft/world/level/storage/McRegionLevelStorageSource.h"
#include "minecraft/world/level/tile/Tile.h"
#include "platform/PlatformTypes.h"
#include "platform/profile/profile.h"
#include "platform/storage/storage.h"
#include "strings.h"
#include "util/Timer.h"
#if defined(SPLIT_SAVES)
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileSplit.h"
#endif
#include "minecraft/world/level/levelgen/ConsoleSchematicFile.h"
#include "minecraft/network/Socket.h"
#include "minecraft/Console_Debug_enum.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/ProgressRenderer.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/server/commands/ServerCommandDispatcher.h"
#include "minecraft/server/level/ServerPlayer.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/biome/BiomeSource.h"
#include "minecraft/world/level/chunk/CompressedTileStorage.h"
#include "minecraft/world/level/chunk/SparseDataStorage.h"
#include "minecraft/world/level/chunk/SparseLightStorage.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/ConsoleSaveFileOriginal.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "platform/ShutdownManager.h"
#include "platform/input/input.h"

class ConsoleInputSource;

#define DEBUG_SERVER_DONT_SPAWN_MOBS 0

// 4J Added
MinecraftServer* MinecraftServer::server = nullptr;
bool MinecraftServer::setTimeAtEndOfTick = false;
int64_t MinecraftServer::setTime = 0;
bool MinecraftServer::setTimeOfDayAtEndOfTick = false;
int64_t MinecraftServer::setTimeOfDay = 0;
bool MinecraftServer::m_bPrimaryPlayerSignedOut = false;
bool MinecraftServer::s_bServerHalted = false;
bool MinecraftServer::s_bSaveOnExitAnswered = false;
#if defined(_ACK_CHUNK_SEND_THROTTLING)
bool MinecraftServer::s_hasSentEnoughPackets = false;
int64_t MinecraftServer::s_tickStartTime = 0;
std::vector<INetworkPlayer*> MinecraftServer::s_sentTo;
#else
int MinecraftServer::s_slowQueuePlayerIndex = 0;
time_util::time_point MinecraftServer::s_slowQueueLastTime = {};
bool MinecraftServer::s_slowQueuePacketSent = false;
#endif

std::unordered_map<std::string, int> MinecraftServer::ironTimers;

MinecraftServer::MinecraftServer() {
    // 4J - added initialisers
    connection = nullptr;
    settings = nullptr;
    players = nullptr;
    commands = nullptr;
    running = true;
    m_bLoaded = false;
    stopped = false;
    tickCount = 0;
    std::string progressStatus;
    progress = 0;
    motd = "";

    m_isServerPaused = false;
    m_serverPausedEvent = new C4JThread::Event;

    m_saveOnExit = false;
    m_suspending = false;

    m_ugcPlayersVersion = 0;
    m_texturePackId = 0;
    maxBuildHeight = Level::maxBuildHeight;
    playerIdleTimeout = 0;
    m_postUpdateThread = nullptr;
    forceGameType = false;

    commandDispatcher = new ServerCommandDispatcher();

    DispenserBootstrap::bootStrap();
}

MinecraftServer::~MinecraftServer() {}

bool MinecraftServer::initServer(int64_t seed, NetworkGameInitData* initData,
                                 std::uint32_t initSettings, bool findSeed) {
    // 4J - removed
    settings = new Settings(new File("server.properties"));

    Log::info("\n*** SERVER SETTINGS ***\n");
    Log::info(
        "ServerSettings: host-friends-only is %s\n",
        (gameServices().getGameHostOption(eGameHostOption_FriendsOfFriends) > 0)
            ? "on"
            : "off");
    Log::info("ServerSettings: game-type is %s\n",
              (gameServices().getGameHostOption(eGameHostOption_GameType) == 0)
                  ? "Survival Mode"
                  : "Creative Mode");
    Log::info("ServerSettings: pvp is %s\n",
              (gameServices().getGameHostOption(eGameHostOption_PvP) > 0)
                  ? "on"
                  : "off");
    Log::info(
        "ServerSettings: fire spreads is %s\n",
        (gameServices().getGameHostOption(eGameHostOption_FireSpreads) > 0)
            ? "on"
            : "off");
    Log::info("ServerSettings: tnt explodes is %s\n",
              (gameServices().getGameHostOption(eGameHostOption_TNT) > 0)
                  ? "on"
                  : "off");
    Log::info("\n");

    // TODO 4J Stu - Init a load of settings based on data passed as params
    // settings->setBooleanAndSave( "host-friends-only",
    // (gameServices().getGameHostOption(eGameHostOption_FriendsOfFriends)>0) );

    // 4J - Unused
    // localIp = settings->getString("server-ip", "");
    // onlineMode = settings->getBoolean("online-mode", true);
    // motd = settings->getString("motd", "A Minecraft Server");
    // motd.replace('§', '$');

    setAnimals(settings->getBoolean("spawn-animals", true));
    setNpcsEnabled(settings->getBoolean("spawn-npcs", true));
    setPvpAllowed(gameServices().getGameHostOption(eGameHostOption_PvP) > 0
                      ? true
                      : false);  // settings->getBoolean("pvp", true);

    // 4J Stu - We should never have hacked clients flying when they shouldn't
    // be like the PC version, so enable flying always Fix for #46612 - TU5:
    // Code: Multiplayer: A client can be banned for flying when accidentaly
    // being blown by dynamite
    setFlightAllowed(true);  // settings->getBoolean("allow-flight", false);

    // 4J Stu - Enabling flight to stop it kicking us when we use it
#if defined(_DEBUG_MENUS_ENABLED)
    setFlightAllowed(true);
#endif

    connection = new ServerConnection(this);
    Socket::Initialise(connection);  // 4J - added
    setPlayers(new PlayerList(this));

    // 4J-JEV: Need to wait for levelGenerationOptions to load.
    while (gameServices().getLevelGenerationOptions() != nullptr &&
           !gameServices().getLevelGenerationOptions()->hasLoadedData())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (gameServices().getLevelGenerationOptions() != nullptr &&
        !gameServices().getLevelGenerationOptions()->ready()) {
        // TODO: Stop loading, add error message.
    }

    int64_t levelNanoTime = System::nanoTime();

    std::string levelName = settings->getString("level-name", "world");
    std::string levelTypeString;

    bool gameRuleUseFlatWorld = false;
    if (gameServices().getLevelGenerationOptions() != nullptr) {
        gameRuleUseFlatWorld =
            gameServices().getLevelGenerationOptions()->getuseFlatWorld();
    }
    if (gameRuleUseFlatWorld ||
        gameServices().getGameHostOption(eGameHostOption_LevelType) > 0) {
        levelTypeString = settings->getString("level-type", "flat");
    } else {
        levelTypeString = settings->getString("level-type", "default");
    }

    LevelType* pLevelType = LevelType::getLevelType(levelTypeString);
    if (pLevelType == nullptr) {
        pLevelType = LevelType::lvl_normal;
    }

    ProgressRenderer* mcprogress = Minecraft::GetInstance()->progressRenderer;
    mcprogress->progressStart(IDS_PROGRESS_INITIALISING_SERVER);

    if (findSeed) {
        seed = BiomeSource::findSeed(pLevelType);
    }

    setMaxBuildHeight(
        settings->getInt("max-build-height", Level::maxBuildHeight));
    setMaxBuildHeight(((getMaxBuildHeight() + 8) / 16) * 16);
    setMaxBuildHeight(
        std::clamp(getMaxBuildHeight(), 64, Level::maxBuildHeight));
    // settings->setProperty("max-build-height", maxBuildHeight);

    //        logger.info("Preparing level \"" + levelName + "\"");
    m_bLoaded = loadLevel(new McRegionLevelStorageSource(File(".")), levelName,
                          seed, pLevelType, initData);
    //        logger.info("Done (" + (System.nanoTime() - levelNanoTime) + "ns)!
    //        For help, type \"help\" or \"?\"");

    // 4J delete passed in save data now - this is only required for the
    // tutorial which is loaded by passing data directly in rather than using
    // the storage manager
    if (initData->saveData) {
        delete[] reinterpret_cast<std::uint8_t*>(initData->saveData->data);
        initData->saveData->data = 0;
        initData->saveData->fileSize = 0;
    }

    NetworkService.ServerReady();  // 4J added
    return m_bLoaded;
}

// 4J - added - extra thread to post processing on separate thread during level
// creation
int MinecraftServer::runPostUpdate(void* lpParam) {
    ShutdownManager::HasStarted(ShutdownManager::ePostProcessThread);

    MinecraftServer* server = (MinecraftServer*)lpParam;
    Entity::useSmallIds();  // This thread can end up spawning entities as
                            // resources
    Compression::UseDefaultThreadStorage();
    Level::enableLightingCache();
    Tile::CreateNewThreadStorage();

    // Update lights for both levels until we are signalled to terminate
    do {
        {
            std::unique_lock<std::mutex> lock(server->m_postProcessCS);
            if (server->m_postProcessRequests.size()) {
                MinecraftServer::postProcessRequest request =
                    server->m_postProcessRequests.back();
                server->m_postProcessRequests.pop_back();
                lock.unlock();
                static int count = 0;
                request.chunkSource->postProcess(request.chunkSource, request.x,
                                                 request.z);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!server->m_postUpdateTerminate &&
             ShutdownManager::ShouldRun(ShutdownManager::ePostProcessThread));
    // #ifndef 0
    //  One final pass through updates to make sure we're done
    {
        std::unique_lock<std::mutex> lock(server->m_postProcessCS);
        int maxRequests = server->m_postProcessRequests.size();
        while (
            server->m_postProcessRequests.size() &&
            ShutdownManager::ShouldRun(ShutdownManager::ePostProcessThread)) {
            MinecraftServer::postProcessRequest request =
                server->m_postProcessRequests.back();
            server->m_postProcessRequests.pop_back();
            lock.unlock();
            request.chunkSource->postProcess(request.chunkSource, request.x,
                                             request.z);
            lock.lock();
        }
    }
    // #endif //0
    Tile::ReleaseThreadStorage();
    Level::destroyLightingCache();

    ShutdownManager::HasFinished(ShutdownManager::ePostProcessThread);

    return 0;
}

void MinecraftServer::addPostProcessRequest(ChunkSource* chunkSource, int x,
                                            int z) {
    {
        std::lock_guard<std::mutex> lock(m_postProcessCS);
        m_postProcessRequests.push_back(
            MinecraftServer::postProcessRequest(x, z, chunkSource));
    }
}

void MinecraftServer::postProcessTerminate(ProgressRenderer* mcprogress) {
    std::uint32_t status = 0;
    size_t postProcessItemCount = 0;
    size_t postProcessItemRemaining = 0;

    {
        std::lock_guard<std::mutex> lock(server->m_postProcessCS);
        postProcessItemCount = server->m_postProcessRequests.size();
    }

    do {
        status = m_postUpdateThread->waitForCompletion(50);
        if (status == C4JThread::WaitResult::Timeout) {
            {
                std::lock_guard<std::mutex> lock(server->m_postProcessCS);
                postProcessItemRemaining = server->m_postProcessRequests.size();
            }

            if (postProcessItemCount) {
                mcprogress->progressStagePercentage(
                    (postProcessItemCount - postProcessItemRemaining) * 100 /
                    postProcessItemCount);
            }
            CompressedTileStorage::tick();
            SparseLightStorage::tick();
            SparseDataStorage::tick();
        }
    } while (status == C4JThread::WaitResult::Timeout);
    delete m_postUpdateThread;
    m_postUpdateThread = nullptr;
}

bool MinecraftServer::loadLevel(LevelStorageSource* storageSource,
                                const std::string& name, int64_t levelSeed,
                                LevelType* pLevelType,
                                NetworkGameInitData* initData) {
    //	4J - TODO - do with new save stuff
    //    if (storageSource->requiresConversion(name))
    //	{
    //		assert(false);
    //    }
    ProgressRenderer* mcprogress = Minecraft::GetInstance()->progressRenderer;

    // 4J TODO - free levels here if there are already some?
    levels = std::vector<ServerLevel*>(3);

    int gameTypeId = settings->getInt(
        "gamemode",
        gameServices().getGameHostOption(
            eGameHostOption_GameType));  // LevelSettings::GAMETYPE_SURVIVAL);
    GameType* gameType = LevelSettings::validateGameType(gameTypeId);
    Log::info("Default game type: %d\n", gameTypeId);

    LevelSettings* levelSettings = new LevelSettings(
        levelSeed, gameType,
        gameServices().getGameHostOption(eGameHostOption_Structures) > 0
            ? true
            : false,
        isHardcore(), true, pLevelType, initData->xzSize, initData->hellScale);
    if (gameServices().getGameHostOption(eGameHostOption_BonusChest))
        levelSettings->enableStartingBonusItems();

    // 4J - temp - load existing level
    std::shared_ptr<McRegionLevelStorage> storage = nullptr;
    bool levelChunksNeedConverted = false;
    if (initData->saveData != nullptr) {
        // We are loading a file from disk with the data passed in

#if defined(SPLIT_SAVES)
        ConsoleSaveFileOriginal oldFormatSave(
            initData->saveData->saveName, initData->saveData->data,
            initData->saveData->fileSize, false, initData->savePlatform);
        ConsoleSaveFile* pSave = new ConsoleSaveFileSplit(&oldFormatSave);

        // ConsoleSaveFile* pSave = new ConsoleSaveFileSplit(
        // initData->saveData->saveName, initData->saveData->data,
        // initData->saveData->fileSize, false, initData->savePlatform );
#else
        ConsoleSaveFile* pSave = new ConsoleSaveFileOriginal(
            initData->saveData->saveName, initData->saveData->data,
            initData->saveData->fileSize, false, initData->savePlatform);
#endif
        if (pSave->isSaveEndianDifferent()) levelChunksNeedConverted = true;
        pSave->ConvertToLocalPlatform();  // check if we need to convert this
                                          // file from PS3->PS4

        storage = std::shared_ptr<McRegionLevelStorage>(
            new McRegionLevelStorage(pSave, File("."), name, true));
    } else {
        // We are loading a save from the storage manager
#if defined(SPLIT_SAVES)
        bool bLevelGenBaseSave = false;
        LevelGenerationOptions* levelGen =
            gameServices().getLevelGenerationOptions();
        if (levelGen != nullptr && levelGen->requiresBaseSave()) {
            unsigned int fileSize = 0;
            std::uint8_t* pvSaveData = levelGen->getBaseSaveData(fileSize);
            if (pvSaveData && fileSize != 0) bLevelGenBaseSave = true;
        }
        ConsoleSaveFileSplit* newFormatSave = nullptr;
        if (bLevelGenBaseSave) {
            ConsoleSaveFileOriginal oldFormatSave("");
            newFormatSave = new ConsoleSaveFileSplit(&oldFormatSave);
        } else {
            newFormatSave = new ConsoleSaveFileSplit("");
        }

        storage = std::shared_ptr<McRegionLevelStorage>(
            new McRegionLevelStorage(newFormatSave, File("."), name, true));
#else
        storage = std::make_shared<McRegionLevelStorage>(
            new ConsoleSaveFileOriginal(""), File("."), name, true);
#endif
    }

    //	McRegionLevelStorage *storage = new McRegionLevelStorage(new
    // ConsoleSaveFile( "" ), "", "", 0); // original
    //    McRegionLevelStorage *storage = new McRegionLevelStorage(File("."),
    //    name, true); // TODO
    for (unsigned int i = 0; i < levels.size(); i++) {
        if (s_bServerHalted || !NetworkService.IsInSession()) {
            return false;
        }

        //            String levelName = name;
        //            if (i == 1) levelName += "_nether";
        int dimension = 0;
        if (i == 1) dimension = -1;
        if (i == 2) dimension = 1;
        if (i == 0) {
            levels[i] =
                new ServerLevel(this, storage, name, dimension, levelSettings);
            if (gameServices().getLevelGenerationOptions() != nullptr) {
                LevelGenerationOptions* mapOptions =
                    gameServices().getLevelGenerationOptions();
                Pos* spawnPos = mapOptions->getSpawnPos();
                if (spawnPos != nullptr) {
                    levels[i]->setSpawnPos(spawnPos);
                }

                levels[i]->getLevelData()->setHasBeenInCreative(
                    mapOptions->isFromDLC());
            }
        } else
            levels[i] = new DerivedServerLevel(this, storage, name, dimension,
                                               levelSettings, levels[0]);
        //        levels[i]->addListener(new ServerLevelListener(this,
        //        levels[i]));		// 4J - have moved this to the
        //        ServerLevel ctor so that it is set up in time for the first
        //        chunk to load, which might actually happen there

        // 4J Stu - We set the levels difficulty based on the minecraft options
        // levels[i]->difficulty = settings->getBoolean("spawn-monsters", true)
        // ? Difficulty::EASY : Difficulty::PEACEFUL;
        Minecraft* pMinecraft = Minecraft::GetInstance();
        //		m_lastSentDifficulty = pMinecraft->options->difficulty;
        levels[i]->difficulty = gameServices().getGameHostOption(
            eGameHostOption_Difficulty);  // pMinecraft->options->difficulty;
        Log::info("MinecraftServer::loadLevel - Difficulty = %d\n",
                  levels[i]->difficulty);

#if DEBUG_SERVER_DONT_SPAWN_MOBS
        levels[i]->setSpawnSettings(false, false);
#else
        levels[i]->setSpawnSettings(
            settings->getBoolean("spawn-monsters", true), animals);
#endif
        levels[i]->getLevelData()->setGameType(gameType);

        if (gameServices().getLevelGenerationOptions() != nullptr) {
            LevelGenerationOptions* mapOptions =
                gameServices().getLevelGenerationOptions();
            levels[i]->getLevelData()->setHasBeenInCreative(
                mapOptions->getLevelHasBeenInCreative());
        }

        players->setLevel(levels);
    }

    if (levels[0]->isNew) {
        mcprogress->progressStage(IDS_PROGRESS_GENERATING_SPAWN_AREA);
    } else {
        mcprogress->progressStage(IDS_PROGRESS_LOADING_SPAWN_AREA);
    }
    gameServices().setGameHostOption(
        eGameHostOption_HasBeenInCreative,
        gameType == GameType::CREATIVE || levels[0]->getHasBeenInCreative());
    gameServices().setGameHostOption(eGameHostOption_Structures,
                                     levels[0]->isGenerateMapFeatures());

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    // 4J - Make a new thread to do post processing

    // 4J-PB - fix for 108310 - TCR #001 BAS Game Stability: TU12: Code:
    // Compliance: Crash after creating world on "journey" seed. Stack gets very
    // deep with some sand tower falling, so increased the stacj to 256K from
    // 128k on other platforms (was already set to that on PS3 and Orbis)

    m_postUpdateThread =
        new C4JThread(runPostUpdate, this, "Post processing", 256 * 1024);

    m_postUpdateTerminate = false;
    m_postUpdateThread->setPriority(C4JThread::ThreadPriority::AboveNormal);
    m_postUpdateThread->run();

    int64_t startTime = System::currentTimeMillis();

    // 4J Stu - Added this to temporarily make starting games on vita faster
    int r = 196;

    //  4J JEV: load gameRules.
    ConsoleSavePath filepath(GAME_RULE_SAVENAME);
    ConsoleSaveFile* csf = getLevel(0)->getLevelStorage()->getSaveFile();
    if (csf->doesFileExist(filepath)) {
        unsigned int numberOfBytesRead;
        std::vector<uint8_t> ba_gameRules;

        FileEntry* fe = csf->createFile(filepath);

        ba_gameRules.resize(fe->getFileSize());

        csf->setFilePointer(fe, 0, SaveFileSeekOrigin::Begin);
        csf->readFile(fe, ba_gameRules.data(), ba_gameRules.size(),
                      &numberOfBytesRead);
        assert(numberOfBytesRead == ba_gameRules.size());

        gameServices().loadGameRules(ba_gameRules.data(), ba_gameRules.size());
        csf->closeHandle(fe);
    }

    int64_t lastTime = System::currentTimeMillis();
#if defined(_LARGE_WORLDS)
    if (gameServices().getGameNewWorldSize() >
        levels[0]->getLevelData()->getXZSizeOld()) {
        if (!gameServices()
                 .getGameNewWorldSizeUseMoat())  // check the moat settings to
                                                 // see if we should be
                                                 // overwriting the edge tiles
        {
            overwriteBordersForNewWorldSize(levels[0]);
        }
        // we're always overwriting hell edges
        int oldHellSize = levels[0]->getLevelData()->getXZHellSizeOld();
        overwriteHellBordersForNewWorldSize(levels[1], oldHellSize);
    }
#endif

    // 4J Stu - This loop is changed in 1.0.1 to only process the first level
    // (ie the overworld), but I think we still want to do them all
    int i = 0;
    for (int i = 0; i < levels.size(); i++) {
        //        logger.info("Preparing start region for level " + i);
        if (i == 0 || settings->getBoolean("allow-nether", true)) {
            ServerLevel* level = levels[i];
            if (levelChunksNeedConverted) {
                // 				storage->getSaveFile()->convertLevelChunks(level)
            }

            int64_t lastStorageTickTime = System::currentTimeMillis();
            Pos* spawnPos = level->getSharedSpawnPos();

            int twoRPlusOne = r * 2 + 1;
            int total = twoRPlusOne * twoRPlusOne;
            for (int x = -r; x <= r && running; x += 16) {
                for (int z = -r; z <= r && running; z += 16) {
                    if (s_bServerHalted || !NetworkService.IsInSession()) {
                        delete spawnPos;
                        m_postUpdateTerminate = true;
                        postProcessTerminate(mcprogress);
                        return false;
                    }
                    //					printf(">>>%d %d
                    //%d\n",i,x,z);
                    //                    int64_t now =
                    //                    System::currentTimeMillis(); if (now <
                    //                    lastTime) lastTime = now; if (now >
                    //                    lastTime + 1000)
                    {
                        int pos = (x + r) * twoRPlusOne + (z + 1);
                        //                        setProgress("Preparing spawn
                        //                        area", (pos) * 100 / total);
                        mcprogress->progressStagePercentage((pos + r) * 100 /
                                                            total);
                        //                        lastTime = now;
                    }
                    static int count = 0;
                    level->cache->create((spawnPos->x + x) >> 4,
                                         (spawnPos->z + z) >> 4,
                                         true);  // 4J - added parameter to
                                                 // disable postprocessing here

                    //                    while (level->updateLights() &&
                    //                    running)
                    //                        ;
                    if (System::currentTimeMillis() - lastStorageTickTime >
                        50) {
                        CompressedTileStorage::tick();
                        SparseLightStorage::tick();
                        SparseDataStorage::tick();
                        lastStorageTickTime = System::currentTimeMillis();
                    }
                }
            }

            // 4J - removed this as now doing the recheckGaps call when each
            // chunk is post-processed, so can happen on things outside of the
            // spawn area too

            delete spawnPos;
        }
    }
    //	printf("Main thread complete at %dms\n",System::currentTimeMillis() -
    // startTime);

    // Wait for post processing, then lighting threads, to end (post-processing
    // may make more lighting changes)
    m_postUpdateTerminate = true;

    postProcessTerminate(mcprogress);

    // stronghold position?
    if (levels[0]->dimension->id == 0) {
        Log::info("===================================\n");

        if (!levels[0]->getLevelData()->getHasStronghold()) {
            int x, z;
            if (gameServices().getTerrainFeaturePosition(
                    eTerrainFeature_Stronghold, &x, &z)) {
                levels[0]->getLevelData()->setXStronghold(x);
                levels[0]->getLevelData()->setZStronghold(z);
                levels[0]->getLevelData()->setHasStronghold();

                Log::info("=== FOUND stronghold in terrain features list\n");

            } else {
                // can't find the stronghold position in the terrain feature
                // list. Do we have to run a post-process?
                Log::info(
                    "=== Can't find stronghold in terrain features list\n");
            }
        } else {
            Log::info("=== Leveldata has stronghold position\n");
        }
        Log::info("===================================\n");
    }

    //	printf("Post processing complete at %dms\n",System::currentTimeMillis()
    //- startTime);

    //	printf("Lighting complete at %dms\n",System::currentTimeMillis() -
    // startTime);

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    if (levels[1]->isNew) {
        levels[1]->save(true, mcprogress);
    }

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    if (levels[2]->isNew) {
        levels[2]->save(true, mcprogress);
    }

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    // 4J - added - immediately save newly created level, like single player
    // game 4J Stu - We also want to immediately save the tutorial
    if (levels[0]->isNew) saveGameRules();

    if (levels[0]->isNew) {
        levels[0]->save(true, mcprogress);
    }

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    if (levels[0]->isNew || levels[1]->isNew || levels[2]->isNew) {
        levels[0]->saveToDisc(mcprogress, false);
    }

    if (s_bServerHalted || !NetworkService.IsInSession()) return false;

    /*
     * int r = 24; for (int x = -r; x <= r; x++) {
     * setProgress("Preparing spawn area", (x + r) * 100 / (r + r + 1)); for
     * (int z = -r; z <= r; z++) { if (!running) return;
     * level.cache.create((level.xSpawn
     * >> 4) + x, (level.zSpawn >> 4) + z); while (running &&
     * level.updateLights()) ; } }
     */
    endProgress();

    return true;
}

#if defined(_LARGE_WORLDS)
void MinecraftServer::overwriteBordersForNewWorldSize(ServerLevel* level) {
    // recreate the chunks round the border (2 chunks or 32 blocks deep),
    // deleting any player data from them
    Log::info("Expanding level size\n");
    int oldSize = level->getLevelData()->getXZSizeOld();
    // top
    int minVal = -oldSize / 2;
    int maxVal = (oldSize / 2) - 1;
    for (int xVal = minVal; xVal <= maxVal; xVal++) {
        int zVal = minVal;
        level->cache->overwriteLevelChunkFromSource(xVal, zVal);
        level->cache->overwriteLevelChunkFromSource(xVal, zVal + 1);
    }
    // bottom
    for (int xVal = minVal; xVal <= maxVal; xVal++) {
        int zVal = maxVal;
        level->cache->overwriteLevelChunkFromSource(xVal, zVal);
        level->cache->overwriteLevelChunkFromSource(xVal, zVal - 1);
    }
    // left
    for (int zVal = minVal; zVal <= maxVal; zVal++) {
        int xVal = minVal;
        level->cache->overwriteLevelChunkFromSource(xVal, zVal);
        level->cache->overwriteLevelChunkFromSource(xVal + 1, zVal);
    }
    // right
    for (int zVal = minVal; zVal <= maxVal; zVal++) {
        int xVal = maxVal;
        level->cache->overwriteLevelChunkFromSource(xVal, zVal);
        level->cache->overwriteLevelChunkFromSource(xVal - 1, zVal);
    }
}

void MinecraftServer::overwriteHellBordersForNewWorldSize(ServerLevel* level,
                                                          int oldHellSize) {
    // recreate the chunks round the border (1 chunk or 16 blocks deep),
    // deleting any player data from them
    Log::info("Expanding level size\n");
    // top
    int minVal = -oldHellSize / 2;
    int maxVal = (oldHellSize / 2) - 1;
    for (int xVal = minVal; xVal <= maxVal; xVal++) {
        int zVal = minVal;
        level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal,
                                                        maxVal);
    }
    // bottom
    for (int xVal = minVal; xVal <= maxVal; xVal++) {
        int zVal = maxVal;
        level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal,
                                                        maxVal);
    }
    // left
    for (int zVal = minVal; zVal <= maxVal; zVal++) {
        int xVal = minVal;
        level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal,
                                                        maxVal);
    }
    // right
    for (int zVal = minVal; zVal <= maxVal; zVal++) {
        int xVal = maxVal;
        level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal,
                                                        maxVal);
    }
}

#endif

void MinecraftServer::setProgress(const std::string& status, int progress) {
    progressStatus = status;
    this->progress = progress;
    //    logger.info(status + ": " + progress + "%");
}

void MinecraftServer::endProgress() {
    progressStatus = "";
    this->progress = 0;
}

void MinecraftServer::saveAllChunks() {
    //    logger.info("Saving chunks");
    for (unsigned int i = 0; i < levels.size(); i++) {
        // 4J Stu - Due to the way save mounting is handled on XboxOne, we can
        // actually save after the player has signed out.
        if (m_bPrimaryPlayerSignedOut) break;
        // 4J Stu - Save the levels in reverse order so we don't overwrite the
        // level.dat with the data from the nethers leveldata. Fix for #7418 -
        // Functional: Gameplay: Saving after sleeping in a bed will place
        // player at nighttime when restarting.
        ServerLevel* level = levels[levels.size() - 1 - i];
        if (level)  // 4J - added check as level can be nullptr if we end up in
                    // stopServer really early on due to network failure
        {
            level->save(true, Minecraft::GetInstance()->progressRenderer);

            // Only close the level storage when we have saved the last level,
            // otherwise we need to recreate the region files when saving the
            // next levels
            if (i == (levels.size() - 1)) {
                level->closeLevelStorage();
            }
        }
    }
}

// 4J-JEV: Added
void MinecraftServer::saveGameRules() {
#if !defined(_CONTENT_PACKAGE)
    if (gameServices().debugSettingsOn() &&
        gameServices().debugGetMask(PlatformInput.GetPrimaryPad()) &
            (1L << eDebugSetting_DistributableSave)) {
        // Do nothing
    } else
#endif
    {
        uint8_t* baPtr = nullptr;
        unsigned int baSize = 0;
        gameServices().saveGameRules(&baPtr, &baSize);

        if (baPtr != nullptr) {
            std::vector<uint8_t> ba(baPtr, baPtr + baSize);
            ConsoleSaveFile* csf =
                getLevel(0)->getLevelStorage()->getSaveFile();
            FileEntry* fe =
                csf->createFile(ConsoleSavePath(GAME_RULE_SAVENAME));
            csf->setFilePointer(fe, 0, SaveFileSeekOrigin::Begin);
            unsigned int length;
            csf->writeFile(fe, ba.data(), ba.size(), &length);

            csf->closeHandle(fe);
        }
    }
}

void MinecraftServer::Suspend() {
    m_suspending = true;
    time_util::Timer timer;
    if (m_bLoaded && (!PlatformStorage.GetSaveDisabled())) {
        if (players != nullptr) {
            players->saveAll(nullptr);
        }
        for (unsigned int j = 0; j < levels.size(); j++) {
            if (s_bServerHalted) break;
            // 4J Stu - Save the levels in reverse order so we don't overwrite
            // the level.dat with the data from the nethers leveldata. Fix for
            // #7418 - Functional: Gameplay: Saving after sleeping in a bed will
            // place player at nighttime when restarting.
            ServerLevel* level = levels[levels.size() - 1 - j];
            level->Suspend();
        }
        if (!s_bServerHalted) {
            saveGameRules();
            levels[0]->saveToDisc(nullptr, true);
        }
    }

    m_suspending = false;
    Log::info("Suspend server: Elapsed time %f\n",
              static_cast<float>(timer.elapsed_seconds()));
}

bool MinecraftServer::IsSuspending() { return m_suspending; }

void MinecraftServer::stopServer(bool didInit) {
    // 4J-PB - need to halt the rendering of the data, since we're about to
    // remove it
    { Minecraft::GetInstance()->gameRenderer->DisableUpdateThread(); }

    connection->stop();

    Log::info("Stopping server\n");
    //    logger.info("Stopping server");
    // 4J-PB - If the primary player has signed out, then don't attempt to save
    // anything

    // also need to check for a profile switch here - primary player signs out,
    // and another player signs in before dismissing the dash
    if ((m_bPrimaryPlayerSignedOut == false) &&
        PlatformProfile.IsSignedIn(PlatformInput.GetPrimaryPad())) {
        // if trial version or saving is disabled, then don't save anything.
        // Also don't save anything if we didn't actually get through the server
        // initialisation.
        if (m_saveOnExit && (!PlatformStorage.GetSaveDisabled()) && didInit) {
            if (players != nullptr) {
                players->saveAll(Minecraft::GetInstance()->progressRenderer,
                                 true);
            }
            // 4J Stu - Save the levels in reverse order so we don't overwrite
            // the level.dat with the data from the nethers leveldata. Fix for
            // #7418 - Functional: Gameplay: Saving after sleeping in a bed will
            // place player at nighttime when restarting.
            // for (unsigned int i = levels.size() - 1; i >= 0; i--)
            //{
            //	ServerLevel *level = levels[i];
            //	if (level != nullptr)
            //	{
            saveAllChunks();
            //	}
            //}

            saveGameRules();
            gameServices().unloadCurrentGameRules();
            if (levels[0] != nullptr)  // This can be null if stopServer happens
                                       // very quickly due to network error
            {
                levels[0]->saveToDisc(
                    Minecraft::GetInstance()->progressRenderer, false);
            }
        }
    }
    // reset the primary player signout flag
    m_bPrimaryPlayerSignedOut = false;
    s_bServerHalted = false;

    // On Durango/Orbis, we need to wait for all the asynchronous saving
    // processes to complete before destroying the levels, as that will
    // ultimately delete the directory level storage & therefore the
    // ConsoleSaveSplit instance, which needs to be around until all the sub
    // files have completed saving.

    // 4J-PB remove the server levels
    unsigned int iServerLevelC = levels.size();
    for (unsigned int i = 0; i < iServerLevelC; i++) {
        if (levels[i] != nullptr) {
            delete levels[i];
            levels[i] = nullptr;
        }
    }

    delete connection;
    connection = nullptr;
    delete players;
    players = nullptr;
    delete settings;
    settings = nullptr;

    NetworkService.ServerStopped();
}

void MinecraftServer::halt() { running = false; }

void MinecraftServer::setMaxBuildHeight(int maxBuildHeight) {
    this->maxBuildHeight = maxBuildHeight;
}

int MinecraftServer::getMaxBuildHeight() { return maxBuildHeight; }

PlayerList* MinecraftServer::getPlayers() { return players; }

void MinecraftServer::setPlayers(PlayerList* players) {
    this->players = players;
}

ServerConnection* MinecraftServer::getConnection() { return connection; }

bool MinecraftServer::isAnimals() { return animals; }

void MinecraftServer::setAnimals(bool animals) { this->animals = animals; }

bool MinecraftServer::isNpcsEnabled() { return npcs; }

void MinecraftServer::setNpcsEnabled(bool npcs) { this->npcs = npcs; }

bool MinecraftServer::isPvpAllowed() { return pvp; }

void MinecraftServer::setPvpAllowed(bool pvp) { this->pvp = pvp; }

bool MinecraftServer::isFlightAllowed() { return allowFlight; }

void MinecraftServer::setFlightAllowed(bool allowFlight) {
    this->allowFlight = allowFlight;
}

bool MinecraftServer::isCommandBlockEnabled() {
    return false;  // settings.getBoolean("enable-command-block", false);
}

bool MinecraftServer::isNetherEnabled() {
    return true;  // settings.getBoolean("allow-nether", true);
}

bool MinecraftServer::isHardcore() { return false; }

int MinecraftServer::getOperatorUserPermissionLevel() {
    return Command::LEVEL_OWNERS;  // settings.getInt("op-permission-level",
                                   // Command.LEVEL_OWNERS);
}

CommandDispatcher* MinecraftServer::getCommandDispatcher() {
    return commandDispatcher;
}

Pos* MinecraftServer::getCommandSenderWorldPosition() {
    return new Pos(0, 0, 0);
}

Level* MinecraftServer::getCommandSenderWorld() { return levels[0]; }

int MinecraftServer::getSpawnProtectionRadius() { return 16; }

bool MinecraftServer::isUnderSpawnProtection(Level* level, int x, int y, int z,
                                             std::shared_ptr<Player> player) {
    if (level->dimension->id != 0) return false;
    // if (getPlayers()->getOps()->empty()) return false;
    if (getPlayers()->isOp(player->getName())) return false;
    if (getSpawnProtectionRadius() <= 0) return false;

    Pos* spawnPos = level->getSharedSpawnPos();
    int xd = std::abs(x - spawnPos->x);
    int zd = std::abs(z - spawnPos->z);
    int dist = std::max(xd, zd);

    return dist <= getSpawnProtectionRadius();
}

void MinecraftServer::setForceGameType(bool forceGameType) {
    this->forceGameType = forceGameType;
}

bool MinecraftServer::getForceGameType() { return forceGameType; }

int64_t MinecraftServer::getCurrentTimeMillis() {
    return System::currentTimeMillis();
}

int MinecraftServer::getPlayerIdleTimeout() { return playerIdleTimeout; }

void MinecraftServer::setPlayerIdleTimeout(int playerIdleTimeout) {
    this->playerIdleTimeout = playerIdleTimeout;
}

extern int c0a, c0b, c1a, c1b, c1c, c2a, c2b;
void MinecraftServer::run(int64_t seed, void* lpParameter) {
    NetworkGameInitData* initData = nullptr;
    std::uint32_t initSettings = 0;
    bool findSeed = false;
    if (lpParameter != nullptr) {
        initData = (NetworkGameInitData*)lpParameter;
        initSettings = gameServices().getGameHostOption(eGameHostOption_All);
        findSeed = initData->findSeed;
        m_texturePackId = initData->texturePackId;
    }
    //    try {		// 4J - removed try/catch/finally
    bool didInit = false;
    if (initServer(seed, initData, initSettings, findSeed)) {
        didInit = true;
        ServerLevel* levelNormalDimension = levels[0];
        // 4J-PB - Set the Stronghold position in the leveldata if there isn't
        // one in there
        Minecraft* pMinecraft = Minecraft::GetInstance();
        LevelData* pLevelData = levelNormalDimension->getLevelData();

        if (pLevelData && pLevelData->getHasStronghold() == false) {
            int x, z;
            if (gameServices().getTerrainFeaturePosition(
                    eTerrainFeature_Stronghold, &x, &z)) {
                pLevelData->setXStronghold(x);
                pLevelData->setZStronghold(z);
                pLevelData->setHasStronghold();
            }
        }

        int64_t lastTime = getCurrentTimeMillis();
        int64_t unprocessedTime = 0;
        while (running && !s_bServerHalted) {
            int64_t now = getCurrentTimeMillis();

            // 4J Stu - When we pause the server, we don't want to count that as
            // time passed 4J Stu - TU-1 hotifx - Remove this line. We want to
            // make sure that we tick connections at the proper rate when paused
            // Fix for #13191 - The host of a game can get a message informing
            // them that the connection to the server has been lost
            // if(m_isServerPaused) lastTime = now;

            int64_t passedTime = now - lastTime;
            if (passedTime > MS_PER_TICK * 40) {
                //                logger.warning("Can't keep up! Did the system
                //                time change, or is the server overloaded?");
                passedTime = MS_PER_TICK * 40;
            }
            if (passedTime < 0) {
                //                logger.warning("Time ran backwards! Did the
                //                system time change?");
                passedTime = 0;
            }
            unprocessedTime += passedTime;
            lastTime = now;

            // 4J Added ability to pause the server
            if (!m_isServerPaused) {
                bool didTick = false;
                if (levels[0]->allPlayersAreSleeping()) {
                    tick();
                    unprocessedTime = 0;
                } else {
                    //					int tickcount = 0;
                    //					int64_t beforeall =
                    // System::currentTimeMillis();
                    while (unprocessedTime > MS_PER_TICK) {
                        unprocessedTime -= MS_PER_TICK;
                        chunkPacketManagement_PreTick();
                        //						int64_t
                        // before = System::currentTimeMillis();
                        tick();
                        //						int64_t
                        // after = System::currentTimeMillis();
                        //						PIXReportCounter("Server
                        // time",(float)(after-before));

                        chunkPacketManagement_PostTick();
                    }
                    //					int64_t afterall =
                    // System::currentTimeMillis();
                    // M_PIXReportCounter("Server time
                    // all",(float)(afterall-beforeall));
                    //					PIXReportCounter("Server
                    // ticks",(float)tickcount);
                }
            } else {
                // 4J Stu - TU1-hotfix
                // Fix for #13191 - The host of a game can get a message
                // informing them that the connection to the server has been
                // lost
                // The connections should tick at the same frequency even when
                // paused
                while (unprocessedTime > MS_PER_TICK) {
                    unprocessedTime -= MS_PER_TICK;
                    // Keep ticking the connections to stop them timing out
                    connection->tick();
                }
            }
            if (MinecraftServer::setTimeAtEndOfTick) {
                MinecraftServer::setTimeAtEndOfTick = false;
                for (unsigned int i = 0; i < levels.size(); i++) {
                    //					if (i == 0 ||
                    // settings->getBoolean("allow-nether", true))
                    //// 4J removed - we always have nether
                    {
                        ServerLevel* level = levels[i];
                        level->setGameTime(MinecraftServer::setTime);
                    }
                }
            }
            if (MinecraftServer::setTimeOfDayAtEndOfTick) {
                MinecraftServer::setTimeOfDayAtEndOfTick = false;
                for (unsigned int i = 0; i < levels.size(); i++) {
                    if (i == 0 || settings->getBoolean("allow-nether", true)) {
                        ServerLevel* level = levels[i];
                        level->setDayTime(MinecraftServer::setTimeOfDay);
                    }
                }
            }

            // Drain typed action queue (queued by app/server consumers
            // via MinecraftServer::queueServerAction).
            drainServerActions();

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    // else
    //{
    //      while (running)
    //	{
    //         handleConsoleInputs();
    //		std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //     }
    // }

    // 4J Stu - Stop the server when the loops complete, as the finally would do
    stopServer(didInit);
    stopped = true;
}

void MinecraftServer::broadcastStartSavingPacket() {
    players->broadcastAll(std::shared_ptr<GameEventPacket>(
        new GameEventPacket(GameEventPacket::START_SAVING, 0)));
    ;
}

void MinecraftServer::broadcastStopSavingPacket() {
    if (!s_bServerHalted) {
        players->broadcastAll(std::shared_ptr<GameEventPacket>(
            new GameEventPacket(GameEventPacket::STOP_SAVING, 0)));
        ;
    }
}

void MinecraftServer::tick() {
    std::vector<std::string> toRemove;
    for (auto it = ironTimers.begin(); it != ironTimers.end(); it++) {
        int t = it->second;
        if (t > 0) {
            ironTimers[it->first] = t - 1;
        } else {
            toRemove.push_back(it->first);
        }
    }
    for (unsigned int i = 0; i < toRemove.size(); i++) {
        ironTimers.erase(toRemove[i]);
    }

    tickCount++;

    // 4J We need to update client difficulty levels based on the servers
    Minecraft* pMinecraft = Minecraft::GetInstance();
    // 4J-PB - sending this on the host changing the difficulty in the menus
    /*	if(m_lastSentDifficulty != pMinecraft->options->difficulty)
    {
    m_lastSentDifficulty = pMinecraft->options->difficulty;
    players->broadcastAll( shared_ptr<ServerSettingsChangedPacket>( new
    ServerSettingsChangedPacket( ServerSettingsChangedPacket::HOST_DIFFICULTY,
    pMinecraft->options->difficulty) ) );
    }*/

    for (unsigned int i = 0; i < levels.size(); i++) {
        //        if (i == 0 || settings->getBoolean("allow-nether", true))
        //        // 4J removed - we always have nether
        {
            ServerLevel* level = levels[i];

            // 4J Stu - We set the levels difficulty based on the minecraft
            // options
            level->difficulty = gameServices().getGameHostOption(
                eGameHostOption_Difficulty);  // pMinecraft->options->difficulty;

#if DEBUG_SERVER_DONT_SPAWN_MOBS
            level->setSpawnSettings(false, false);
#else
            level->setSpawnSettings(level->difficulty > 0 &&
                                        !Minecraft::GetInstance()->isTutorial(),
                                    animals);
#endif

            if (tickCount % 20 == 0) {
                players->broadcastAll(
                    std::make_shared<SetTimePacket>(
                        level->getGameTime(), level->getDayTime(),
                        level->getGameRules()->getBoolean(
                            GameRules::RULE_DAYLIGHT)),
                    level->dimension->id);
            }
            // #ifndef 0
            static int64_t stc = 0;
            int64_t st0 = System::currentTimeMillis();
            ((Level*)level)->tick();
            int64_t st1 = System::currentTimeMillis();

            int64_t st2 = System::currentTimeMillis();

            // 4J added to stop ticking entities in levels when players are not
            // in those levels. Note: now changed so that we also tick if there
            // are entities to be removed, as this also happens as a result of
            // calling tickEntities. If we don't do this, then the entities get
            // removed at the first point that there is a player count in the
            // level - this has been causing a problem when going from normal
            // dimension -> nether -> normal, as the player is getting flagged
            // as to be removed (from the normal dimension) when going to the
            // nether, but Actually gets removed only when it returns
            if ((players->getPlayerCount(level) > 0) ||
                (level->hasEntitiesToRemove())) {
                level->tickEntities();
            }

            level->getTracker()->tick();

            int64_t st3 = System::currentTimeMillis();
            //			printf(">>>>>>>>>>>>>>>>>>>>>> Tick %d %d %d :
            //%d\n", st1 - st0, st2 - st1, st3 - st2, st0 - stc );
            stc = st0;
            // #endif// 0
        }
    }
    Entity::tickExtraWandering();  // 4J added

    connection->tick();

    players->tick();

    // 4J - removed

    //    try {		// 4J - removed try/catch
    handleConsoleInputs();
    //    } catch (Exception e) {
    //        logger.log(Level.WARNING, "Unexpected exception while parsing
    //        console command", e);
    //    }
}

void MinecraftServer::handleConsoleInput(const std::string& msg,
                                         ConsoleInputSource* source) {
    consoleInput.push_back(new ConsoleInput(msg, source));
}

void MinecraftServer::handleConsoleInputs() {
    while (consoleInput.size() > 0) {
        auto it = consoleInput.begin();
        ConsoleInput* input = *it;
        consoleInput.erase(it);
        //        commands->handleCommand(input);		// 4J - removed
        //        - TODO - do we want equivalent of console commands?
    }
}

void MinecraftServer::main(int64_t seed, void* lpParameter) {
    ShutdownManager::HasStarted(ShutdownManager::eServerThread);
    server = new MinecraftServer();
    server->run(seed, lpParameter);
    delete server;
    server = nullptr;
    ShutdownManager::HasFinished(ShutdownManager::eServerThread);
}

void MinecraftServer::HaltServer(bool bPrimaryPlayerSignedOut) {
    s_bServerHalted = true;
    if (server != nullptr) {
        m_bPrimaryPlayerSignedOut = bPrimaryPlayerSignedOut;
        server->halt();
    }
}

File* MinecraftServer::getFile(const std::string& name) {
    return new File(name);
}

void MinecraftServer::info(const std::string& string) {}

void MinecraftServer::warn(const std::string& string) {}

std::string MinecraftServer::getConsoleName() { return "CONSOLE"; }

ServerLevel* MinecraftServer::getLevel(int dimension) {
    if (dimension == -1)
        return levels[1];
    else if (dimension == 1)
        return levels[2];
    else
        return levels[0];
}

// 4J added
void MinecraftServer::setLevel(int dimension, ServerLevel* level) {
    if (dimension == -1)
        levels[1] = level;
    else if (dimension == 1)
        levels[2] = level;
    else
        levels[0] = level;
}

#if defined(_ACK_CHUNK_SEND_THROTTLING)
bool MinecraftServer::chunkPacketManagement_CanSendTo(INetworkPlayer* player) {
    if (s_hasSentEnoughPackets) return false;
    if (player == nullptr) return false;

    for (int i = 0; i < s_sentTo.size(); i++) {
        if (s_sentTo[i]->IsSameSystem(player)) {
            return false;
        }
    }

    return (player->GetOutstandingAckCount() < 2);
}

void MinecraftServer::chunkPacketManagement_DidSendTo(INetworkPlayer* player) {
    int64_t currentTime = System::currentTimeMillis();

    if ((currentTime - s_tickStartTime) >= MAX_TICK_TIME_FOR_PACKET_SENDS) {
        s_hasSentEnoughPackets = true;
        //		Log::info("Sending, setting enough packet flag:
        //%dms\n",currentTime - s_tickStartTime);
    } else {
        //		Log::info("Sending, more time: %dms\n",currentTime
        //- s_tickStartTime);
    }

    player->SentChunkPacket();

    s_sentTo.push_back(player);
}

void MinecraftServer::chunkPacketManagement_PreTick() {
    //	Log::info("*************************************************************************************************************************************************************************\n");
    s_hasSentEnoughPackets = false;
    s_tickStartTime = System::currentTimeMillis();
    s_sentTo.clear();

    std::vector<std::shared_ptr<PlayerConnection>>* players =
        connection->getPlayers();

    if (players->size()) {
        std::vector<std::shared_ptr<PlayerConnection>> playersOrig = *players;
        players->clear();

        do {
            int longestTime = 0;
            auto playerConnectionBest = playersOrig.begin();
            for (auto it = playersOrig.begin(); it != playersOrig.end(); it++) {
                int thisTime = 0;
                INetworkPlayer* np = (*it)->getNetworkPlayer();
                if (np) {
                    thisTime = np->GetTimeSinceLastChunkPacket_ms();
                }

                if (thisTime > longestTime) {
                    playerConnectionBest = it;
                    longestTime = thisTime;
                }
            }
            players->push_back(*playerConnectionBest);
            playersOrig.erase(playerConnectionBest);
        } while (playersOrig.size() > 0);
    }
}

void MinecraftServer::chunkPacketManagement_PostTick() {}

#else
// 4J Added
bool MinecraftServer::chunkPacketManagement_CanSendTo(INetworkPlayer* player) {
    if (player == nullptr) return false;

    auto now = time_util::clock::now();
    if (player->GetSessionIndex() == s_slowQueuePlayerIndex &&
        (now - s_slowQueueLastTime) >
            std::chrono::milliseconds(MINECRAFT_SERVER_SLOW_QUEUE_DELAY)) {
        //		Log::info("Slow queue OK for player #%d\n",
        // player->GetSessionIndex());
        return true;
    }

    return false;
}

void MinecraftServer::chunkPacketManagement_DidSendTo(INetworkPlayer* player) {
    s_slowQueuePacketSent = true;
}

void MinecraftServer::chunkPacketManagement_PreTick() {}

void MinecraftServer::chunkPacketManagement_PostTick() {
    // 4J Ensure that the slow queue owner keeps cycling if it's not been used
    // in a while
    auto now = time_util::clock::now();
    if ((s_slowQueuePacketSent) ||
        ((now - s_slowQueueLastTime) >
         std::chrono::milliseconds(2 * MINECRAFT_SERVER_SLOW_QUEUE_DELAY))) {
        //		Log::info("Considering cycling: (%d) %d - %d -> %d
        //> %d\n",s_slowQueuePacketSent, time, s_slowQueueLastTime, (time -
        // s_slowQueueLastTime), (2*MINECRAFT_SERVER_SLOW_QUEUE_DELAY));
        MinecraftServer::cycleSlowQueueIndex();
        s_slowQueuePacketSent = false;
        s_slowQueueLastTime = now;
    }
    //	else
    //	{
    //		Log::info("Not considering cycling: %d - %d -> %d >
    //%d\n",time, s_slowQueueLastTime, (time - s_slowQueueLastTime),
    //(2*MINECRAFT_SERVER_SLOW_QUEUE_DELAY));
    //	}
}

void MinecraftServer::cycleSlowQueueIndex() {
    if (!NetworkService.IsInSession()) return;

    int startingIndex = s_slowQueuePlayerIndex;
    INetworkPlayer* currentPlayer = nullptr;
    int currentPlayerCount = 0;
    do {
        currentPlayerCount = NetworkService.GetPlayerCount();
        if (startingIndex >= currentPlayerCount) startingIndex = 0;
        ++s_slowQueuePlayerIndex;

        if (currentPlayerCount > 0) {
            s_slowQueuePlayerIndex %= currentPlayerCount;
            // Fix for #9530 - NETWORKING: Attempting to fill a multiplayer game
            // beyond capacity results in a softlock for the last players to
            // join. The QNet session might be ending while we do this, so do a
            // few more checks that the player is real
            currentPlayer =
                NetworkService.GetPlayerByIndex(s_slowQueuePlayerIndex);
        } else {
            s_slowQueuePlayerIndex = 0;
        }
    } while (NetworkService.IsInSession() && currentPlayerCount > 0 &&
             s_slowQueuePlayerIndex != startingIndex &&
             currentPlayer != nullptr && currentPlayer->IsLocal());
    //	Log::info("Cycled slow queue index to %d\n",
    // s_slowQueuePlayerIndex);
}
#endif

// 4J added - sets up a vector of flags to indicate which entities (with small
// Ids) have been removed from the level, but are still haven't constructed a
// network packet to tell a remote client about it. These small Ids shouldn't be
// re-used. Most of the time this method shouldn't actually do anything, in
// which case it will return false and nothing is set up.
bool MinecraftServer::flagEntitiesToBeRemoved(unsigned int* flags) {
    bool removedFound = false;
    for (unsigned int i = 0; i < levels.size(); i++) {
        ServerLevel* level = levels[i];
        if (level) {
            level->flagEntitiesToBeRemoved(flags, &removedFound);
        }
    }
    return removedFound;
}

void MinecraftServer::queueServerAction(
    minecraft::server::ServerAction action) {
    std::lock_guard<std::mutex> lock(m_actionQueueMutex);
    m_actionQueue.push_back(std::move(action));
}

void MinecraftServer::drainServerActions() {
    std::vector<minecraft::server::ServerAction> queue;
    {
        std::lock_guard<std::mutex> lock(m_actionQueueMutex);
        if (m_actionQueue.empty()) return;
        queue.swap(m_actionQueue);
    }
    for (auto& action : queue) {
        std::visit([this](auto& a) { handleServerAction(a); }, action);
    }
}

void MinecraftServer::handleServerAction(const minecraft::server::SaveGame& a) {
    gameServices().lockSaveNotification();
    if (players != nullptr) {
        players->saveAll(Minecraft::GetInstance()->progressRenderer);
    }

    players->broadcastAll(
        std::shared_ptr<UpdateProgressPacket>(new UpdateProgressPacket(20)));

    for (unsigned int j = 0; j < levels.size(); j++) {
        if (s_bServerHalted) break;
        // 4J Stu - Save the levels in reverse order so we don't overwrite
        // the level.dat with the data from the nethers leveldata. Fix for
        // #7418 - Functional: Gameplay: Saving after sleeping in a bed will
        // place player at nighttime when restarting.
        ServerLevel* level = levels[levels.size() - 1 - j];
        level->save(true, Minecraft::GetInstance()->progressRenderer,
                    a.autoSave);

        players->broadcastAll(std::shared_ptr<UpdateProgressPacket>(
            new UpdateProgressPacket(33 + (j * 33))));
    }
    if (!s_bServerHalted) {
        saveGameRules();
        levels[0]->saveToDisc(Minecraft::GetInstance()->progressRenderer,
                              a.autoSave);
    }
    gameServices().unlockSaveNotification();
}

void MinecraftServer::handleServerAction(
    const minecraft::server::DropDebugItem& a) {
    if (players == nullptr || players->players.empty()) return;
    std::shared_ptr<ServerPlayer> player = players->players.at(a.playerIndex);
    player->drop(
        std::shared_ptr<ItemInstance>(new ItemInstance(a.itemId, 1, 0)));
}

void MinecraftServer::handleServerAction(
    const minecraft::server::SpawnDebugMob& a) {
    if (players == nullptr || players->players.empty()) return;
    std::shared_ptr<ServerPlayer> player = players->players.at(a.playerIndex);
    auto factory = static_cast<eINSTANCEOF>(a.mobFactoryId);
    std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(
        EntityIO::newByEnumType(factory, player->level));
    if (mob == nullptr) return;
    mob->moveTo(player->x + 1, player->y, player->z + 1,
                player->level->random->nextFloat() * 360, 0);
    mob->setDespawnProtected();  // 4J added, default to being protected against
                                 // despawning (has to be done after initial
                                 // position is set)
    player->level->addEntity(mob);
}

void MinecraftServer::handleServerAction(
    const minecraft::server::PauseServer& a) {
    m_isServerPaused = a.paused;
    if (m_isServerPaused) {
        m_serverPausedEvent->set();
    }
}

void MinecraftServer::handleServerAction(const minecraft::server::ToggleRain&) {
    bool isRaining = levels[0]->getLevelData()->isRaining();
    levels[0]->getLevelData()->setRaining(!isRaining);
    levels[0]->getLevelData()->setRainTime(
        levels[0]->random->nextInt(Level::TICKS_PER_DAY * 7) +
        Level::TICKS_PER_DAY / 2);
}

void MinecraftServer::handleServerAction(
    const minecraft::server::ToggleThunder&) {
    bool isThundering = levels[0]->getLevelData()->isThundering();
    levels[0]->getLevelData()->setThundering(!isThundering);
    levels[0]->getLevelData()->setThunderTime(
        levels[0]->random->nextInt(Level::TICKS_PER_DAY * 7) +
        Level::TICKS_PER_DAY / 2);
}

void MinecraftServer::handleServerAction(
    const minecraft::server::BroadcastSettingChanged& a) {
    using Kind = minecraft::server::BroadcastSettingChanged::Kind;
    switch (a.kind) {
        case Kind::Gamertags:
            players->broadcastAll(std::shared_ptr<ServerSettingsChangedPacket>(
                new ServerSettingsChangedPacket(
                    ServerSettingsChangedPacket::HOST_OPTIONS,
                    gameServices().getGameHostOption(
                        eGameHostOption_Gamertags))));
            break;
        case Kind::BedrockFog:
            players->broadcastAll(std::shared_ptr<ServerSettingsChangedPacket>(
                new ServerSettingsChangedPacket(
                    ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS,
                    gameServices().getGameHostOption(eGameHostOption_All))));
            break;
        case Kind::Difficulty:
            players->broadcastAll(std::shared_ptr<ServerSettingsChangedPacket>(
                new ServerSettingsChangedPacket(
                    ServerSettingsChangedPacket::HOST_DIFFICULTY,
                    Minecraft::GetInstance()->options->difficulty)));
            break;
    }
}

void MinecraftServer::handleServerAction(
    const minecraft::server::ExportSchematic& a) {
#if !defined(_CONTENT_PACKAGE)
    gameServices().lockSaveNotification();
    if (!s_bServerHalted) {
        File targetFileDir("Schematics");
        if (!targetFileDir.exists()) targetFileDir.mkdir();

        char filename[128];
        snprintf(filename, 128, "%s%dx%dx%d.sch", a.name,
                 (a.endX - a.startX + 1), (a.endY - a.startY + 1),
                 (a.endZ - a.startZ + 1));

        File dataFile = File(targetFileDir, std::string(filename));
        if (dataFile.exists()) dataFile._delete();
        FileOutputStream fos = FileOutputStream(dataFile);
        DataOutputStream dos = DataOutputStream(&fos);
        ConsoleSchematicFile::generateSchematicFile(
            &dos, levels[0], a.startX, a.startY, a.startZ, a.endX, a.endY,
            a.endZ, a.saveMobs, a.compressionType);
        dos.close();
    }
    gameServices().unlockSaveNotification();
#else
    (void)a;
#endif
}

void MinecraftServer::handleServerAction(
    const minecraft::server::SetCameraLocation& a) {
#if !defined(_CONTENT_PACKAGE)
    Log::info("DEBUG: Player=%i\n", a.playerIndex);
    Log::info(
        "DEBUG: Teleporting to pos=(%f.2, %f.2, %f.2), looking "
        "at=(%f.2,%f.2)\n",
        a.x, a.y, a.z, a.yRot, a.elev);

    if (players == nullptr ||
        a.playerIndex >= static_cast<int>(players->players.size()))
        return;
    std::shared_ptr<ServerPlayer> player = players->players.at(a.playerIndex);
    player->debug_setPosition(a.x, a.y, a.z, a.yRot, a.elev);
#else
    (void)a;
#endif
}
