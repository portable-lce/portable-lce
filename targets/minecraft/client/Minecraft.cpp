#include "Minecraft.h"

#include <assert.h>
#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <thread>

#include "Options.h"
#include "Pos.h"
#include "ProgressRenderer.h"
#include "SharedConstants.h"
#include "Timer.h"
#include "User.h"
#include "app/common/Audio/SoundEngine.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/linux/Linux_UIController.h"
#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/GameEnums.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/IMenuService.h"
#include "minecraft/client/gui/DeathScreen.h"
#include "minecraft/client/gui/ErrorScreen.h"
#include "minecraft/client/gui/PauseScreen.h"
#include "minecraft/client/gui/Screen.h"
#include "minecraft/client/gui/inventory/InventoryScreen.h"
#include "minecraft/client/gui/particle/GuiParticles.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/multiplayer/MultiPlayerLevel.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/particle/ParticleEngine.h"
#include "minecraft/client/player/LocalPlayer.h"
#include "minecraft/client/renderer/Chunk.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/client/renderer/LevelRenderer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/client/title/TitleScreen.h"
#include "minecraft/network/INetworkService.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "minecraft/network/packet/Packet.h"
#include "minecraft/network/platform/NetworkPlayerInterface.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/stats/Stats.h"
#include "minecraft/stats/StatsCounter.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/ItemFrame.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/animal/Animal.h"
#include "minecraft/world/entity/animal/EntityHorse.h"
#include "minecraft/world/entity/animal/Ocelot.h"
#include "minecraft/world/entity/animal/Pig.h"
#include "minecraft/world/entity/animal/Sheep.h"
#include "minecraft/world/entity/animal/Wolf.h"
#include "minecraft/world/entity/monster/Spider.h"
#include "minecraft/world/entity/monster/Zombie.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/entity/player/SkinTypes.h"
#include "minecraft/world/food/FoodData.h"
#include "minecraft/world/item/DyePowderItem.h"
#include "minecraft/world/item/FoodItem.h"
#include "minecraft/world/item/GoldenAppleItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/item/LeashItem.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/CompressedTileStorage.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/dlc/DLCConstants.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/storage/LevelStorageSource.h"
#include "minecraft/world/level/storage/McRegionLevelStorageSource.h"
#include "minecraft/world/level/tile/ChestTile.h"
#include "minecraft/world/level/tile/ColoredTile.h"
#include "minecraft/world/level/tile/TallGrassPlantTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/phys/HitResult.h"
#include "platform/XboxStubs.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"
#include "platform/stubs.h"
#include "strings.h"
#if defined(ENABLE_JAVA_GUIS)
#include "minecraft/client/gui/inventory/CreativeInventoryScreen.h"
#endif
#include "app/common/ConsoleGameMode.h"
#include "app/common/DLC/DLCPack.h"
#include "app/common/Tutorial/FullTutorialMode.h"
#include "app/common/UI/All Platforms/IUIScene_CreativeMenu.h"
#include "app/common/UI/UIFontData.h"
#include "java/File.h"
#include "java/System.h"
#include "minecraft/Minecraft_Macros.h"
#include "minecraft/StaticConstructors.h"
#include "minecraft/client/MemoryTracker.h"
#include "minecraft/client/gui/Font.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/gui/InBedChatScreen.h"
#include "minecraft/client/gui/ScreenSizeCalculator.h"
#include "minecraft/client/gui/achievement/AchievementPopup.h"
#include "minecraft/client/multiplayer/ClientConnection.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/client/player/Input.h"
#include "minecraft/client/renderer/texture/TextureManager.h"
#include "minecraft/client/resources/Colours/ColourTable.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/world/entity/npc/Villager.h"
#include "minecraft/world/item/alchemy/PotionMacros.h"
#include "minecraft/world/level/chunk/SparseDataStorage.h"
#include "minecraft/world/level/chunk/SparseLightStorage.h"
#include "platform/input/input.h"
#include "util/StringHelpers.h"

class ChunkSource;

// #define DISABLE_SPU_CODE
// 4J Turning this on will change the graph at the bottom of the debug overlay
// to show the number of packets of each type added per fram
// #define DEBUG_RENDER_SHOWS_PACKETS 1
// #define SPLITSCREEN_TEST

// If not disabled, this creates an event queue on a seperate thread so that the
// Level::tick calls can be offloaded from the main thread, and have longer to
// run, since it's called at 20Hz instead of 60
#define DISABLE_LEVELTICK_THREAD

Minecraft* Minecraft::m_instance = nullptr;
int64_t Minecraft::frameTimes[512];
int64_t Minecraft::tickTimes[512];
int Minecraft::frameTimePos = 0;
int64_t Minecraft::warezTime = 0;
File Minecraft::workDir = File("");

ResourceLocation Minecraft::DEFAULT_FONT_LOCATION =
    ResourceLocation(TN_DEFAULT_FONT);
ResourceLocation Minecraft::ALT_FONT_LOCATION = ResourceLocation(TN_ALT_FONT);

Minecraft::Minecraft(Component* mouseComponent, Canvas* parent,
                     MinecraftApplet* minecraftApplet, int width, int height,
                     bool fullscreen, IPlatformLeaderboard& leaderboard_)
    : leaderboard(leaderboard_) {
    // 4J - added this block of initialisers
    gameMode = nullptr;
    hasCrashed = false;
    timer = new Timer(SharedConstants::TICKS_PER_SECOND);
    oldLevel = nullptr;  // 4J Stu added
    level = nullptr;
    levels = std::vector<MultiPlayerLevel*>(3);  // 4J Added
    levelRenderer = nullptr;
    player = nullptr;
    cameraTargetPlayer = nullptr;
    particleEngine = nullptr;
    user = nullptr;
    parent = nullptr;
    pause = false;
    exitingWorldRightNow = false;
    textures = nullptr;
    font = nullptr;
    screen = nullptr;
    localPlayerIdx = 0;
    rightClickDelay = 0;

    // 4J Stu Added
    // m_hPlayerRespawned = CreateEvent(nullptr, false, false, nullptr);

    progressRenderer = nullptr;
    gameRenderer = nullptr;
    bgLoader = nullptr;

    ticks = 0;
    // 4J-PB - moved into the local player
    // missTime = 0;
    // lastClickTick = 0;
    // isRaining = false;
    // 4J-PB - end

    orgWidth = orgHeight = 0;
    achievementPopup = new AchievementPopup(this);
    gui = nullptr;
    noRender = false;
    humanoidModel = new HumanoidModel(0);
    hitResult = 0;
    options = nullptr;
    soundEngine = new SoundEngine();
    mouseHandler = nullptr;
    skins = nullptr;
    workingDirectory = File("");
    levelSource = nullptr;
    stats[0] = nullptr;
    stats[1] = nullptr;
    stats[2] = nullptr;
    stats[3] = nullptr;
    connectToPort = 0;
    workDir = File("");
    // 4J removed
    // wasDown = false;
    lastTimer = -1;

    // 4J removed
    // lastTickTime = System::currentTimeMillis();
    recheckPlayerIn = 0;
    running = true;
    unoccupiedQuadrant = -1;

    Stats::init();

    orgHeight = height;
    this->fullscreen = fullscreen;
    this->minecraftApplet = nullptr;

    this->parent = parent;
    // 4J - Our actual physical frame buffer is always 1280x720 ie in a 16:9
    // ratio. If we want to do a 4:3 mode, we are telling the original minecraft
    // code that the width is 3/4 what it actually is, to correctly present a
    // 4:3 image. Have added width_phys and height_phys for any code we add that
    // requires to know the real physical dimensions of the frame buffer.
    if (PlatformRenderer.IsWidescreen()) {
        this->width = width;
    } else {
        this->width = (width * 3) / 4;
    }
    this->height = height;
    this->width_phys = width;
    this->height_phys = height;

    this->fullscreen = fullscreen;

    appletMode = false;

    Minecraft::m_instance = this;
    TextureManager::createInstance();

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        m_pendingLocalConnections[i] = nullptr;
        m_connectionFailed[i] = false;
        localgameModes[i] = nullptr;
    }

    animateTickLevel = nullptr;  // 4J added
    m_inFullTutorialBits = 0;    // 4J Added
    reloadTextures = false;

    // initialise the audio before any textures are loaded - to avoid the
    // problem in win64 of the Miles audio causing the codec for textures to be
    // unloaded

    // 4J-PB - Removed it from here on Orbis due to it causing a crash with the
    // network init. We should work out why...
    this->soundEngine->init(nullptr);

#if !defined(DISABLE_LEVELTICK_THREAD)
    levelTickEventQueue =
        new C4JThread::EventQueue(levelTickUpdateFunc, levelTickThreadInitFunc,
                                  "LevelTick_EventQueuePoll");
    levelTickEventQueue->setPriority(C4JThread::ThreadPriority::Normal);
#endif
}

void Minecraft::clearConnectionFailed() {
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        m_connectionFailed[i] = false;
        m_connectionFailedReason[i] = DisconnectPacket::eDisconnect_None;
    }
    gameServices().setDisconnectReason(DisconnectPacket::eDisconnect_None);
}

void Minecraft::connectTo(const std::string& server, int port) {
    connectToIp = server;
    connectToPort = port;
}

void Minecraft::init() {
    // glClearColor(0.2f, 0.2f, 0.2f, 1);

    workingDirectory = getWorkingDirectory();
    levelSource =
        new McRegionLevelStorageSource(File(workingDirectory, "saves"));
    //        levelSource = new MemoryLevelStorageSource();
    options = new Options(this, workingDirectory);
    skins = new TexturePackRepository(workingDirectory, this);
    skins->addDebugPacks();
    textures = new Textures(skins, options);
    // renderLoadingScreen();

    font =
        new Font(options, "font/Default.png", textures, false,
                 &DEFAULT_FONT_LOCATION, 23, 20, 8, 8, SFontData::Codepoints);
    altFont = new Font(options, "font/alternate.png", textures, false,
                       &ALT_FONT_LOCATION, 16, 16, 8, 8);

    // if (options.languageCode != null) {
    //	Language.getInstance().loadLanguage(options.languageCode);
    //	//
    // font.setEnforceUnicodeSheet("true".equalsIgnoreCase(I18n.get("language.enforceUnicode")));
    //	font.setEnforceUnicodeSheet(Language.getInstance().isSelectedLanguageIsUnicode());
    //	font.setBidirectional(Language.isBidirectional(options.languageCode));
    // }

    // 4J Stu - Not using these any more
    // WaterColor::init(textures->loadTexturePixels("misc/watercolor.png"));
    // GrassColor::init(textures->loadTexturePixels("misc/grasscolor.png"));
    // FoliageColor::init(textures->loadTexturePixels("misc/foliagecolor.png"));

    gameRenderer = new GameRenderer(this);
    EntityRenderDispatcher::instance->itemInHandRenderer =
        new ItemInHandRenderer(this, false);

    for (int i = 0; i < 4; ++i) stats[i] = new StatsCounter(leaderboard);

    /*		4J - TODO, 4J-JEV: Unnecessary.
    Achievements::openInventory->setDescFormatter(nullptr);
    Achievements.openInventory.setDescFormatter(new DescFormatter(){
    public String format(String i18nValue) {
    return String.format(i18nValue, Keyboard.getKeyName(options.keyBuild.key));
    }
    });
    */

    // 4J-PB - We'll do this in a xui intro
    // renderLoadingScreen();

    // Keyboard::create();
    Mouse::create();

    checkGlError("Pre startup");

    // width = Display.getDisplayMode().getWidth();
    // height = Display.getDisplayMode().getHeight();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glCullFace(GL_BACK);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    checkGlError("Startup");

    //    openGLCapabilities = new OpenGLCapabilities();	// 4J - removed

    levelRenderer = new LevelRenderer(this, textures);
    // textures->register(&TextureAtlas::LOCATION_BLOCKS, new
    // TextureAtlas(Icon::TYPE_TERRAIN, TN_TERRAIN));
    // textures->register(&TextureAtlas::LOCATION_ITEMS, new
    // TextureAtlas(Icon::TYPE_ITEM, TN_GUI_ITEMS));
    textures->stitch();

    glViewport(0, 0, width, height);

    particleEngine = new ParticleEngine(level, textures);
    //    try {	// 4J - removed try/catch
    bgLoader = new BackgroundDownloader(workingDirectory, this);
    bgLoader->start();
    //    } catch (Exception e) {
    //    }

    checkGlError("Post startup");
    gui = new Gui(this);

    if (connectToIp != "")  // 4J - was nullptr comparison
    {
        //        setScreen(new ConnectScreen(this, connectToIp,
        //        connectToPort));		// 4J TODO - put back in
    } else {
        setScreen(new TitleScreen());
    }
    progressRenderer = new ProgressRenderer(this);

    PlatformRenderer.CBuffLockStaticCreations();
}

void Minecraft::renderLoadingScreen() {
    // 4J Unused
    // testing stuff on vita just now
#if defined(ENABLE_JAVA_GUIS)
    ScreenSizeCalculator ssc(options, width, height);

    // xxx
    PlatformRenderer.StartFrame();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (float)ssc.rawWidth, (float)ssc.rawHeight, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2000);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);

    Tesselator* t = Tesselator::getInstance();

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    // xxx
    glBindTexture(GL_TEXTURE_2D, textures->loadTexture(TN_MOB_PIG));
    t->begin();
    t->color(0xffffff);
    t->vertexUV((float)(0), (float)(height), (float)(0), (float)(0),
                (float)(0));
    t->vertexUV((float)(width), (float)(height), (float)(0), (float)(0),
                (float)(0));
    t->vertexUV((float)(width), (float)(0), (float)(0), (float)(0), (float)(0));
    t->vertexUV((float)(0), (float)(0), (float)(0), (float)(0), (float)(0));
    t->end();

    int lw = 256;
    int lh = 256;
    glColor4f(1, 1, 1, 1);
    t->color(0xffffff);
    blit((ssc.getWidth() - lw) / 2, (ssc.getHeight() - lh) / 2, 0, 0, lw, lh);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    // Display::swapBuffers();
    // xxx
    PlatformRenderer.Present();
#endif
}

void Minecraft::blit(int x, int y, int sx, int sy, int w, int h) {
    float us = 1 / 256.0f;
    float vs = 1 / 256.0f;
    Tesselator* t = Tesselator::getInstance();
    t->begin();
    t->vertexUV((float)(x + 0), (float)(y + h), (float)(0),
                (float)((sx + 0) * us), (float)((sy + h) * vs));
    t->vertexUV((float)(x + w), (float)(y + h), (float)(0),
                (float)((sx + w) * us), (float)((sy + h) * vs));
    t->vertexUV((float)(x + w), (float)(y + 0), (float)(0),
                (float)((sx + w) * us), (float)((sy + 0) * vs));
    t->vertexUV((float)(x + 0), (float)(y + 0), (float)(0),
                (float)((sx + 0) * us), (float)((sy + 0) * vs));
    t->end();
}

File Minecraft::getWorkingDirectory() {
    if (workDir.getPath().empty()) workDir = getWorkingDirectory("4jcraft");
    return workDir;
}

File Minecraft::getWorkingDirectory(const std::string& applicationName) {
    // 4J - original version
    // 4jcraft: ported to C++
    std::string userHome = getenv("HOME");
    File* workingDirectory;
#if defined(__linux__)
    workingDirectory = new File(userHome, '.' + applicationName + '/');
#elif defined(_WINDOWS64)
    std::string applicationData = getenv("APPDATA");
    if (!applicationData.empty()) {
        workingDirectory =
            new File(applicationData, '.' + applicationName + '/');
    } else {
        workingDirectory = new File(userHome, '.' + applicationName + '/');
    }
// #elif defined(_MACOS)
//		workingDirectory = new File(userHome, "Library/Application
// Support/" + applicationName);
#else
    workingDirectory = new File(userHome, applicationName + '/');
#endif
    if (!workingDirectory->exists()) {
        if (!workingDirectory->mkdirs()) {
            Log::info("The working directory could not be created");
            assert(0);
            // throw new RuntimeException("The working directory could not be
            // created: " + workingDirectory);
        }
    }
    return *workingDirectory;
}

LevelStorageSource* Minecraft::getLevelSource() { return levelSource; }

void Minecraft::setScreen(Screen* screen) {
    if (dynamic_cast<ErrorScreen*>(this->screen) != nullptr) return;

    if (this->screen != nullptr) {
        this->screen->removed();
    }

    // 4J Gordon: Do not force a stats save here
    /*if (dynamic_cast<TitleScreen *>(screen)!=nullptr)
    {
    stats->forceSend();
    }
    stats->forceSave();*/

    if (screen == nullptr && level == nullptr) {
        screen = new TitleScreen();
    } else if (player != nullptr &&
               !ui.GetMenuDisplayed(player->GetXboxPad()) &&
               player->getHealth() <= 0) {
#if defined(ENABLE_JAVA_GUIS)
        screen = new DeathScreen();
#else
        // 4J Stu - If we exit from the death screen then we are saved as being
        // dead. In the Java game when you load the game you are still dead, but
        // this is silly so only show the dead screen if we have died during
        // gameplay
        if (ticks == 0) {
            player->respawn();
        } else {
            ui.NavigateToScene(player->GetXboxPad(), eUIScene_DeathMenu,
                               nullptr);
        }
#endif
    }
    this->screen = screen;

    if (dynamic_cast<TitleScreen*>(screen) != nullptr) {
        options->renderDebug = false;
        gui->clearMessages();
    }

    if (screen != nullptr) {
        //        releaseMouse();	// 4J - removed
        ScreenSizeCalculator ssc(options, width, height);
        int screenWidth = ssc.getWidth();
        int screenHeight = ssc.getHeight();
        screen->init(this, screenWidth, screenHeight);
        noRender = false;
    } else {
        //        grabMouse();	// 4J - removed
    }

    // 4J-PB - if a screen has been set, go into menu mode
    // it's possible that player doesn't exist here yet
    // 4jcraft: reuse this for the java GUI
#if defined(ENABLE_JAVA_GUIS)
    if (screen != nullptr && player != nullptr) {
        if (player && player->GetXboxPad() != -1) {
            PlatformInput.SetMenuDisplayed(player->GetXboxPad(), true);
        }
    } else if (player != nullptr) {
        if (player && player->GetXboxPad() != -1) {
            PlatformInput.SetMenuDisplayed(player->GetXboxPad(), false);
        }
    }
#endif
}

void Minecraft::checkGlError(const std::string& string) {
    // 4J - TODO
}

void Minecraft::destroy() {
    // 4J Gordon: Do not force a stats save here
    /*stats->forceSend();
    stats->forceSave();*/

    // 4J - all try/catch/finally things in here removed
    //    try {
    if (this->bgLoader != nullptr) {
        bgLoader->halt();
    }
    //    } catch (Exception e) {
    //    }

    //    try {
    setLevel(nullptr);
    //    } catch (Throwable e) {
    //    }

    if (screen == nullptr && level == nullptr) {
        screen = new TitleScreen();
    } else if (player != nullptr &&
               !ui.GetMenuDisplayed(player->GetXboxPad()) &&
               player->getHealth() <= 0) {
#if defined(ENABLE_JAVA_GUIS)
        screen = new DeathScreen();
#else
        // 4J Stu - If we exit from the death screen then we are saved as being
        // dead. In the Java game when you load the game you are still dead, but
        // this is silly so only show the dead screen if we have died during
        // gameplay
        if (ticks == 0) {
            player->respawn();
        } else {
            ui.NavigateToScene(player->GetXboxPad(), eUIScene_DeathMenu,
                               nullptr);
        }
#endif
    }

    if (screen != nullptr && dynamic_cast<TitleScreen*>(screen) != nullptr) {
        options->renderDebug = false;
        gui->clearMessages();
    }

    if (screen != nullptr) {
        //        releaseMouse();	// 4J - removed
        ScreenSizeCalculator ssc(options, width, height);
        int screenWidth = ssc.getWidth();
        int screenHeight = ssc.getHeight();
        screen->init(this, screenWidth, screenHeight);
        noRender = false;
    } else {
        //        grabMouse();	// 4J - removed
    }

    // 4J-PB - if a screen has been set, go into menu mode
    // it's possible that player doesn't exist here yet
#if defined(ENABLE_JAVA_GUIS)
    if (screen != nullptr) {
        if (player && player->GetXboxPad() != -1) {
            PlatformInput.SetMenuDisplayed(player->GetXboxPad(), true);
        }
    } else {
        if (player && player->GetXboxPad() != -1) {
            PlatformInput.SetMenuDisplayed(player->GetXboxPad(), false);
        }
    }
#endif
    //    try {
    MemoryTracker::release();
    //    } catch (Throwable e) {
    //    }

    soundEngine->destroy();
    Mouse::destroy();
    Keyboard::destroy();
    //} finally {
    Display::destroy();
    //    if (!hasCrashed) System.exit(0);	//4J - removed
    //}
    // System.gc();	// 4J - removed
}

// 4J-PB - splitting this function into 3 parts, so we can call the middle part
// from our xbox game loop

void Minecraft::run() {
    running = true;
    //    try {	// 4J - removed try/catch
    init();
    //    } catch (Exception e) {
    //        e.printStackTrace();
    //       crash(new CrashReport("Failed to start game", e));
    //        return;
    //    }
    //    try {	// 4J - removed try/catch
}

// 4J added - Selects which local player is currently active for processing by
// the existing minecraft code
bool Minecraft::setLocalPlayerIdx(int idx) {
    localPlayerIdx = idx;
    // If the player is not null, but the game mode is then this is just a temp
    // player whose only real purpose is to hold the viewport position
    if (localplayers[idx] == nullptr || localgameModes[idx] == nullptr)
        return false;

    gameMode = localgameModes[idx];
    player = localplayers[idx];
    cameraTargetPlayer = localplayers[idx];
    gameRenderer->itemInHandRenderer = localitemInHandRenderers[idx];
    level = getLevel(localplayers[idx]->dimension);
    particleEngine->setLevel(level);

    return true;
}

int Minecraft::getLocalPlayerIdx() { return localPlayerIdx; }

void Minecraft::updatePlayerViewportAssignments() {
    unoccupiedQuadrant = -1;
    // Find out how many viewports we'll be needing
    int viewportsRequired = 0;
    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        if (localplayers[i] != nullptr) viewportsRequired++;
    }
    if (viewportsRequired == 3) viewportsRequired = 4;

    // Allocate away...
    if (viewportsRequired == 1) {
        // Single viewport
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (localplayers[i] != nullptr)
                localplayers[i]->m_iScreenSection =
                    IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;
        }
    } else if (viewportsRequired == 2) {
        // Split screen - TODO - option for vertical/horizontal split
        int found = 0;
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (localplayers[i] != nullptr) {
                // Primary player settings decide what the mode is
                if (gameServices().getGameSettings(
                        PlatformInput.GetPrimaryPad(),
                        eGameSetting_SplitScreenVertical)) {
                    localplayers[i]->m_iScreenSection =
                        IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT + found;
                } else {
                    localplayers[i]->m_iScreenSection =
                        IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP + found;
                }
                found++;
            }
        }
    } else if (viewportsRequired >= 3) {
        // Quadrants - this is slightly more complicated. We don't want to move
        // viewports around if we are going from 3 to 4, or 4 to 3 players, so
        // persist any allocations for quadrants that already exist.
        bool quadrantsAllocated[4] = {false, false, false, false};

        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (localplayers[i] != nullptr) {
                // 4J Stu - If the game hasn't started, ignore current
                // allocations (as the players won't have seen them) This fixes
                // an issue with the primary player being the 4th controller
                // quadrant, but ending up in the 3rd viewport.
                if (gameServices().getGameStarted()) {
                    if ((localplayers[i]->m_iScreenSection >=
                         IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT) &&
                        (localplayers[i]->m_iScreenSection <=
                         IPlatformRenderer::
                             VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT)) {
                        quadrantsAllocated
                            [localplayers[i]->m_iScreenSection -
                             IPlatformRenderer::
                                 VIEWPORT_TYPE_QUADRANT_TOP_LEFT] = true;
                    }
                } else {
                    // Reset the viewport so that it can be assigned in the next
                    // loop
                    localplayers[i]->m_iScreenSection =
                        IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;
                }
            }
        }

        // Found which quadrants are currently in use, now allocate out any
        // spares that are required
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (localplayers[i] != nullptr) {
                if ((localplayers[i]->m_iScreenSection <
                     IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT) ||
                    (localplayers[i]->m_iScreenSection >
                     IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT)) {
                    for (int j = 0; j < 4; j++) {
                        if (!quadrantsAllocated[j]) {
                            localplayers[i]->m_iScreenSection =
                                IPlatformRenderer::
                                    VIEWPORT_TYPE_QUADRANT_TOP_LEFT +
                                j;
                            quadrantsAllocated[j] = true;
                            break;
                        }
                    }
                }
            }
        }
        // If there's an unoccupied quadrant, record which one so we can clear
        // it to black when rendering
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (quadrantsAllocated[i] == false) {
                unoccupiedQuadrant = i;
            }
        }
    }

    // 4J Stu - If the game is not running we do not want to do this yet, and
    // should wait until the task that caused the app to not be running is
    // finished
    if (gameServices().getGameStarted()) ui.UpdatePlayerBasePositions();
}

// Add a temporary player so that the viewports get re-arranged, and add the
// player to the game session
bool Minecraft::addLocalPlayer(int idx) {
    // int iLocalPlayerC=app.GetLocalPlayerCount();
    if (m_pendingLocalConnections[idx] != nullptr) {
        // 4J Stu - Should we ever be in a state where this happens?
        assert(false);
        m_pendingLocalConnections[idx]->close();
    }
    m_connectionFailed[idx] = false;
    m_pendingLocalConnections[idx] = nullptr;

    bool success = NetworkService.AddLocalPlayerByUserIndex(idx);

    if (success) {
        Log::info("Adding temp local player on pad %d\n", idx);
        localplayers[idx] = std::shared_ptr<MultiplayerLocalPlayer>(
            new MultiplayerLocalPlayer(this, level, user, nullptr));
        localgameModes[idx] = nullptr;

        updatePlayerViewportAssignments();

        ConnectionProgressParams* param = new ConnectionProgressParams();
        param->iPad = idx;
        param->stringId = IDS_PROGRESS_CONNECTING;
        param->showTooltips = true;
        param->setFailTimer = true;
        param->timerTime = CONNECTING_PROGRESS_CHECK_TIME;

        // Joining as second player so always the small progress
        ui.NavigateToScene(idx, eUIScene_ConnectingProgress, param);

    } else {
        Log::info("NetworkService.AddLocalPlayerByUserIndex failed\n");
    }

    return success;
}

void Minecraft::addPendingLocalConnection(int idx,
                                          ClientConnection* connection) {
    m_pendingLocalConnections[idx] = connection;
}

std::shared_ptr<MultiplayerLocalPlayer> Minecraft::createExtraLocalPlayer(
    int idx, const std::string& name, int iPad, int iDimension,
    ClientConnection* clientConnection /*= nullptr*/,
    MultiPlayerLevel* levelpassedin) {
    if (clientConnection == nullptr) return nullptr;

    if (clientConnection == m_pendingLocalConnections[idx]) {
        int tempScreenSection = IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;
        if (localplayers[idx] != nullptr && localgameModes[idx] == nullptr) {
            // A temp player displaying a connecting screen
            tempScreenSection = localplayers[idx]->m_iScreenSection;
        }
        std::string prevname = user->name;
        user->name = name;

        // Don't need this any more
        m_pendingLocalConnections[idx] = nullptr;

        // Add the connection to the level which will now take responsibility
        // for ticking it 4J-PB - can't use the dimension from
        // localplayers[idx], since there may be no localplayers at this point
        // MultiPlayerLevel *mpLevel = (MultiPlayerLevel *)getLevel(
        // localplayers[idx]->dimension );

        MultiPlayerLevel* mpLevel;

        if (levelpassedin) {
            level = levelpassedin;
            mpLevel = levelpassedin;
        } else {
            level = getLevel(iDimension);
            mpLevel = getLevel(iDimension);
            mpLevel->addClientConnection(clientConnection);
        }

        if (gameServices().getTutorialMode()) {
            localgameModes[idx] =
                new FullTutorialMode(idx, this, clientConnection);
        } else {
            localgameModes[idx] =
                new ConsoleGameMode(idx, this, clientConnection);
        }

        // 4J-PB - can't do this here because they use a render context, but
        // this is running from a thread. Moved the creation of these into the
        // main thread, before level launch
        // localitemInHandRenderers[idx] = new ItemInHandRenderer(this);
        localplayers[idx] = localgameModes[idx]->createPlayer(level);

        PlayerUID playerXUIDOffline = INVALID_XUID;
        PlayerUID playerXUIDOnline = INVALID_XUID;
        PlatformProfile.GetXUID(idx, &playerXUIDOffline, false);
        PlatformProfile.GetXUID(idx, &playerXUIDOnline, true);
        localplayers[idx]->setXuid(playerXUIDOffline);
        localplayers[idx]->setOnlineXuid(playerXUIDOnline);
        localplayers[idx]->setIsGuest(PlatformProfile.IsGuest(idx));

        localplayers[idx]->m_displayName = PlatformProfile.GetDisplayName(idx);

        localplayers[idx]->m_iScreenSection = tempScreenSection;

        if (levelpassedin == nullptr)
            level->addEntity(
                localplayers[idx]);  // Don't add if we're passing the level in,
                                     // we only do this from the client
                                     // connection & we'll be handling adding it
                                     // ourselves

        localplayers[idx]->SetXboxPad(iPad);

        if (localplayers[idx]->input != nullptr)
            delete localplayers[idx]->input;
        localplayers[idx]->input = new Input();

        localplayers[idx]->resetPos();

        levelRenderer->setLevel(idx, level);
        localplayers[idx]->level = level;

        user->name = prevname;

        updatePlayerViewportAssignments();

        // Fix for #105852 - TU12: Content: Gameplay: Local splitscreen Players
        // are spawned at incorrect places after re-joining previously saved and
        // loaded "Mass Effect World". Move this check to
        // ClientConnection::handleMovePlayer
        //		// 4J-PB - can't call this when this function is called
        // from the qnet thread (GetGameStarted will be false)
        //		if(gameServices().getGameStarted())
        //		{
        //			ui.CloseUIScenes(idx);
        //		}
    }

    return localplayers[idx];
}

// on a respawn of the local player, just store them
void Minecraft::storeExtraLocalPlayer(int idx) {
    localplayers[idx] = player;

    if (localplayers[idx]->input != nullptr) delete localplayers[idx]->input;
    localplayers[idx]->input = new Input();

    if (PlatformProfile.IsSignedIn(idx)) {
        localplayers[idx]->name = PlatformProfile.GetGamertag(idx);
    }
}

void Minecraft::removeLocalPlayerIdx(int idx) {
    bool updateXui = true;
    if (localgameModes[idx] != nullptr) {
        if (getLevel(localplayers[idx]->dimension)->isClientSide) {
            std::shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[idx];
            ((MultiPlayerLevel*)getLevel(localplayers[idx]->dimension))
                ->removeClientConnection(mplp->connection, true);
            delete mplp->connection;
            mplp->connection = nullptr;
            NetworkService.RemoveLocalPlayerByUserIndex(idx);
        }
        getLevel(localplayers[idx]->dimension)->removeEntity(localplayers[idx]);

        // 4J Stu - Fix for #13257 - CRASH: Gameplay: Title crashed after
        // exiting the tutorial It doesn't matter if they were in the tutorial
        // already
        playerLeftTutorial(idx);

        delete localgameModes[idx];
        localgameModes[idx] = nullptr;
    } else if (m_pendingLocalConnections[idx] != nullptr) {
        m_pendingLocalConnections[idx]->sendAndDisconnect(
            std::shared_ptr<DisconnectPacket>(
                new DisconnectPacket(DisconnectPacket::eDisconnect_Quitting)));
        ;
        delete m_pendingLocalConnections[idx];
        m_pendingLocalConnections[idx] = nullptr;
        NetworkService.RemoveLocalPlayerByUserIndex(idx);
    } else {
        // Not sure how this works on qnet, but for other platforms, calling
        // RemoveLocalPlayerByUserIndex won't do anything if there isn't a local
        // user to remove Now just updating the UI directly in this case
        // 4J Stu - Adding this back in for exactly the reason my comment above
        // suggests it was added in the first place
    }
    localplayers[idx] = nullptr;

    if (idx == PlatformInput.GetPrimaryPad()) {
        // We should never try to remove the Primary player in this way
        assert(false);
        /*
        // If we are removing the primary player then there can't be a valid
        gamemode left anymore, this
        // pointer will be referring to the one we've just deleted
        gameMode = nullptr;
        // Remove references to player
        player = nullptr;
        cameraTargetPlayer = nullptr;
        EntityRenderDispatcher::instance->cameraEntity = nullptr;
        TileEntityRenderDispatcher::instance->cameraEntity = nullptr;
        */
    } else if (updateXui) {
        gameRenderer->DisableUpdateThread();
        levelRenderer->setLevel(idx, nullptr);
        gameRenderer->EnableUpdateThread();
        ui.CloseUIScenes(idx, true);
        updatePlayerViewportAssignments();
    }

    // We only create these once ever so don't delete it here
    // delete localitemInHandRenderers[idx];
}

void Minecraft::createPrimaryLocalPlayer(int iPad) {
    localgameModes[iPad] = gameMode;
    localplayers[iPad] = player;
    // gameRenderer->itemInHandRenderer = localitemInHandRenderers[iPad];
    //  Give them the gamertag if they're signed in
    if (PlatformProfile.IsSignedIn(PlatformInput.GetPrimaryPad())) {
        user->name = PlatformProfile.GetGamertag(PlatformInput.GetPrimaryPad());
    }
}

void Minecraft::run_middle() {
    static int64_t lastTime = 0;
    static bool bFirstTimeIntoGame = true;
    static bool bAutosaveTimerSet = false;
    static unsigned int uiAutosaveTimer = 0;
    static int iFirstTimeCountdown = 60;
    if (lastTime == 0) lastTime = System::nanoTime();
    static int frames = 0;

#if defined(ENABLE_JAVA_GUIS)
    // 4jcraft: while the java ui is leaving world, don't run the rest of
    // run_middle
    if (exitingWorldRightNow) {
        screen->render(0, 0, 1);
        return;
    }
#endif

    {
        std::lock_guard<std::recursive_mutex> lock(m_setLevelCS);

        if (running) {
            if (reloadTextures) {
                reloadTextures = false;
                textures->reloadAll();
            }

            // while (running)
            {
                //        try {	// 4J - removed try/catch
                //            if (minecraftApplet != null &&
                //            !minecraftApplet.isActive()) break;	// 4J -
                //            removed

                //            if (parent == nullptr &&
                //            Display.isCloseRequested()) {
                //            // 4J - removed
                //                stop();
                //            }

                // 4J-PB - AUTOSAVE TIMER - if the player is the host
                if (level != nullptr && NetworkService.IsHost()) {
                    /*if(!bAutosaveTimerSet)
                    {
                    // set the timer
                    bAutosaveTimerSet=true;

                    gameServices().setAutosaveTimerTime();
                    }
                    else*/
                    {
                        // if the pause menu is up for the primary player, don't
                        // autosave If saving isn't disabled, and the main
                        // player has a app action running , or has any crafting
                        // or containers open, don't autosave
                        if (!PlatformStorage.GetSaveDisabled() &&
                            (gameServices().getXuiAction(
                                 PlatformInput.GetPrimaryPad()) ==
                             eAppAction_Idle)) {
                            if (!ui.IsPauseMenuDisplayed(
                                    PlatformInput.GetPrimaryPad()) &&
                                !ui.IsIgnoreAutosaveMenuDisplayed(
                                    PlatformInput.GetPrimaryPad())) {
                                // check if the autotimer countdown has reached
                                // zero
                                unsigned char ucAutosaveVal =
                                    gameServices().getGameSettings(
                                        PlatformInput.GetPrimaryPad(),
                                        eGameSetting_Autosave);
                                bool bTrialTexturepack = false;
                                if (!Minecraft::GetInstance()
                                         ->skins->isUsingDefaultSkin()) {
                                    TexturePack* tPack =
                                        Minecraft::GetInstance()
                                            ->skins->getSelected();
                                    DLCTexturePack* pDLCTexPack =
                                        (DLCTexturePack*)tPack;

                                    DLCPack* pDLCPack =
                                        pDLCTexPack->getDLCInfoParentPack();

                                    if (pDLCPack) {
                                        if (!pDLCPack->hasPurchasedFile(
                                                DLCManager::e_DLCType_Texture,
                                                "")) {
                                            bTrialTexturepack = true;
                                        }
                                    }
                                }

                                // If the autosave value is not zero, and the
                                // player isn't using a trial texture pack, then
                                // check whether we need to save this tick
                                if ((ucAutosaveVal != 0) &&
                                    !bTrialTexturepack) {
                                    if (gameServices().autosaveDue()) {
                                        // disable the autosave countdown
                                        ui.ShowAutosaveCountdownTimer(false);

                                        // Need to save now
                                        Log::info("+++++++++++\n");
                                        Log::info("+++Autosave\n");
                                        Log::info("+++++++++++\n");
                                        gameServices().setAction(
                                            PlatformInput.GetPrimaryPad(),
                                            eAppAction_AutosaveSaveGame);
                                        // gameServices().setAutosaveTimerTime();
#if !defined(_CONTENT_PACKAGE)
                                        {
                                            // print the time
                                            auto now_tp = std::chrono::
                                                system_clock::now();
                                            std::time_t now_tt = std::chrono::
                                                system_clock::to_time_t(now_tp);
                                            std::tm utcTime{};
#if defined(_WIN32)
                                            gmtime_s(&utcTime, &now_tt);
#else
                                            gmtime_r(&now_tt, &utcTime);
#endif

                                            Log::info("%02d:%02d:%02d\n",
                                                      utcTime.tm_hour,
                                                      utcTime.tm_min,
                                                      utcTime.tm_sec);
                                        }
#endif
                                    } else {
                                        int64_t uiTimeToAutosave =
                                            gameServices().secondsToAutosave();

                                        if (uiTimeToAutosave < 6) {
                                            ui.ShowAutosaveCountdownTimer(true);
                                            ui.UpdateAutosaveCountdownTimer(
                                                uiTimeToAutosave);
                                        }
                                    }
                                }
                            } else {
                                // disable the autosave countdown
                                ui.ShowAutosaveCountdownTimer(false);
                            }
                        }
                    }
                }

                // 4J-PB - Once we're in the level, check if the players have
                // the level in their banned list and ask if they want to play
                // it
                for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                    if (localplayers[i] &&
                        (gameServices().getBanListCheck(i) == false) &&
                        !Minecraft::GetInstance()->isTutorial() &&
                        PlatformProfile.IsSignedInLive(i) &&
                        !PlatformProfile.IsGuest(i)) {
                        // If there is a sys ui displayed, we can't display the
                        // message box here, so ignore until we can
                        if (!PlatformProfile.IsSystemUIDisplayed()) {
                            gameServices().setBanListCheck(i, true);
                            // 4J-PB - check if the level is in the banned level
                            // list get the unique save name and xuid from
                            // whoever is the host
                            INetworkPlayer* pHostPlayer =
                                NetworkService.GetHostPlayer();
                            PlayerUID xuid = pHostPlayer->GetUID();

                            if (gameServices().isInBannedLevelList(
                                    i, xuid,
                                    gameServices().getUniqueMapName())) {
                                // put up a message box asking if the player
                                // would like to unban this level
                                Log::info("This level is banned\n");
                                // set the app action to bring up the message
                                // box to give them the option to remove from
                                // the ban list or exit the level
                                gameServices().setAction(
                                    i, eAppAction_LevelInBanLevelList,
                                    (void*)true);
                            }
                        }
                    }
                }

                if (!PlatformProfile.IsSystemUIDisplayed() &&
                    gameServices().dlcInstallProcessCompleted() &&
                    !gameServices().dlcInstallPending() &&
                    gameServices().dlcNeedsCorruptCheck()) {
                    gameServices().dlcCheckForCorrupt();
                }

                // When we go into the first loaded level, check if the console
                // has active joypads that are not in the game, and bring up the
                // quadrant display to remind them to press start (if the
                // session has space)
                if (level != nullptr && bFirstTimeIntoGame &&
                    NetworkService.SessionHasSpace()) {
                    // have a short delay before the display
                    if (iFirstTimeCountdown == 0) {
                        bFirstTimeIntoGame = false;

                        if (gameServices().isLocalMultiplayerAvailable()) {
                            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                                if ((localplayers[i] == nullptr) &&
                                    PlatformInput.IsPadConnected(i)) {
                                    if (!ui.PressStartPlaying(i)) {
                                        ui.ShowPressStart(i);
                                    }
                                }
                            }
                        }
                    } else
                        iFirstTimeCountdown--;
                }
                // 4J-PB - store any button toggles for the players, since the
                // minecraft::tick may not be called if we're running fast, and
                // a button press and release will be missed

                for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                    if (localplayers[i]) {
                        // 4J-PB - add these to check for coming out of idle
                        if (PlatformInput.ButtonPressed(i,
                                                        MINECRAFT_ACTION_JUMP))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_JUMP;
                        if (PlatformInput.ButtonPressed(i,
                                                        MINECRAFT_ACTION_USE))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_USE;

                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_INVENTORY))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_INVENTORY;
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_ACTION))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_ACTION;
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_CRAFTING))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_CRAFTING;
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_PAUSEMENU)) {
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_PAUSEMENU;
                            Log::info(
                                "PAUSE PRESSED - ipad = %d, Storing press\n",
                                i);
#if defined(ENABLE_JAVA_GUIS)
                            pauseGame();
#endif
                        }
                        if (PlatformInput.ButtonPressed(i,
                                                        MINECRAFT_ACTION_DROP))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_DROP;

                        // 4J-PB - If we're flying, the sneak needs to be held
                        // on to go down
                        if (localplayers[i]->abilities.flying) {
                            if (PlatformInput.ButtonDown(
                                    i, MINECRAFT_ACTION_SNEAK_TOGGLE))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_SNEAK_TOGGLE;
                        } else {
                            if (PlatformInput.ButtonPressed(
                                    i, MINECRAFT_ACTION_SNEAK_TOGGLE))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_SNEAK_TOGGLE;
                        }
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_RENDER_THIRD_PERSON))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_RENDER_THIRD_PERSON;
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_GAME_INFO))
                            localplayers[i]->ullButtonsPressed |=
                                1LL << MINECRAFT_ACTION_GAME_INFO;

#if !defined(_FINAL_BUILD)
                        if (gameServices().debugSettingsOn() &&
                            gameServices().getUseDPadForDebug()) {
                            localplayers[i]->ullDpad_last = 0;
                            localplayers[i]->ullDpad_this = 0;
                            localplayers[i]->ullDpad_filtered = 0;
                            if (PlatformInput.ButtonPressed(
                                    i, MINECRAFT_ACTION_DPAD_RIGHT))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_CHANGE_SKIN;
                            if (PlatformInput.ButtonPressed(
                                    i, MINECRAFT_ACTION_DPAD_UP))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_FLY_TOGGLE;
                            if (PlatformInput.ButtonPressed(
                                    i, MINECRAFT_ACTION_DPAD_DOWN))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_RENDER_DEBUG;
                            if (PlatformInput.ButtonPressed(
                                    i, MINECRAFT_ACTION_DPAD_LEFT))
                                localplayers[i]->ullButtonsPressed |=
                                    1LL << MINECRAFT_ACTION_SPAWN_CREEPER;
                        } else
#endif
                        {
                            // Movement on DPAD is stored ulimately into
                            // ullDpad_filtered - this ignores any diagonals
                            // pressed, instead reporting the last single
                            // direction
                            // - otherwise we get loads of accidental diagonal
                            // movements

                            localplayers[i]->ullDpad_this = 0;
                            int dirCount = 0;

                            if (PlatformInput.ButtonDown(
                                    i, MINECRAFT_ACTION_DPAD_LEFT)) {
                                localplayers[i]->ullDpad_this |=
                                    1LL << MINECRAFT_ACTION_DPAD_LEFT;
                                dirCount++;
                            }
                            if (PlatformInput.ButtonDown(
                                    i, MINECRAFT_ACTION_DPAD_RIGHT)) {
                                localplayers[i]->ullDpad_this |=
                                    1LL << MINECRAFT_ACTION_DPAD_RIGHT;
                                dirCount++;
                            }
                            if (PlatformInput.ButtonDown(
                                    i, MINECRAFT_ACTION_DPAD_UP)) {
                                localplayers[i]->ullDpad_this |=
                                    1LL << MINECRAFT_ACTION_DPAD_UP;
                                dirCount++;
                            }
                            if (PlatformInput.ButtonDown(
                                    i, MINECRAFT_ACTION_DPAD_DOWN)) {
                                localplayers[i]->ullDpad_this |=
                                    1LL << MINECRAFT_ACTION_DPAD_DOWN;
                                dirCount++;
                            }

                            if (dirCount <= 1) {
                                localplayers[i]->ullDpad_last =
                                    localplayers[i]->ullDpad_this;
                                localplayers[i]->ullDpad_filtered =
                                    localplayers[i]->ullDpad_this;
                            } else {
                                localplayers[i]->ullDpad_filtered =
                                    localplayers[i]->ullDpad_last;
                            }
                        }

                        // for the opacity timer
                        if (PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_LEFT_SCROLL) ||
                            PlatformInput.ButtonPressed(
                                i, MINECRAFT_ACTION_RIGHT_SCROLL))
                        // PlatformInput.ButtonPressed(i, MINECRAFT_ACTION_USE)
                        // || PlatformInput.ButtonPressed(i,
                        // MINECRAFT_ACTION_ACTION))
                        {
                            gameServices().setOpacityTimer(i);
                        }
                    } else {
                        // 4J Stu - This doesn't make any sense with the way we
                        // handle XboxOne users
                        // did we just get input from a player who doesn't
                        // exist? They'll be wanting to join the game then
                        bool tryJoin = !pause &&
                                       !ui.IsIgnorePlayerJoinMenuDisplayed(
                                           PlatformInput.GetPrimaryPad()) &&
                                       NetworkService.SessionHasSpace() &&
                                       PlatformRenderer.IsHiDef() &&
                                       PlatformInput.ButtonPressed(i);
                        if (tryJoin) {
                            if (!ui.PressStartPlaying(i)) {
                                ui.ShowPressStart(i);
                            } else {
                                // did we just get input from a player who
                                // doesn't exist? They'll be wanting to join the
                                // game then
                                if (PlatformInput.ButtonPressed(
                                        i, MINECRAFT_ACTION_PAUSEMENU)) {
                                    // Let them join

                                    // are they signed in?
                                    if (PlatformProfile.IsSignedIn(i)) {
                                        // if this is a local game, then the
                                        // player just needs to be signed in
                                        if (NetworkService.IsLocalGame() ||
                                            (PlatformProfile.IsSignedInLive(
                                                 i) &&
                                             PlatformProfile
                                                 .AllowedToPlayMultiplayer(
                                                     i))) {
                                            if (level->isClientSide) {
                                                bool success =
                                                    addLocalPlayer(i);

                                                if (!success) {
                                                    Log::info(
                                                        "Bringing up the sign "
                                                        "in "
                                                        "ui\n");
                                                    PlatformProfile.RequestSignInUI(
                                                        false,
                                                        NetworkService
                                                            .IsLocalGame(),
                                                        true, false, true,
                                                        [this](bool b, int p) {
                                                            return InGame_SignInReturned(
                                                                this, b, p);
                                                        },
                                                        i);
                                                } else {
                                                }
                                            } else {
                                                // create the localplayer
                                                std::shared_ptr<Player> player =
                                                    localplayers[i];
                                                if (player == nullptr) {
                                                    player =
                                                        createExtraLocalPlayer(
                                                            i,
                                                            PlatformProfile
                                                                .GetGamertag(i),
                                                            i,
                                                            level->dimension
                                                                ->id);
                                                }
                                            }
                                        } else {
                                            if (PlatformProfile.IsSignedInLive(
                                                    PlatformProfile
                                                        .GetPrimaryPad()) &&
                                                !PlatformProfile
                                                     .AllowedToPlayMultiplayer(
                                                         i)) {
                                                PlatformProfile
                                                    .RequestConvertOfflineToGuestUI(
                                                        [this](bool b, int p) {
                                                            return InGame_SignInReturned(
                                                                this, b, p);
                                                        },
                                                        i);
                                                // 4J Stu - Don't allow
                                                // converting to guests as we
                                                // don't allow any guest sign-in
                                                // while in the game Fix for
                                                // #66516 - TCR #124: MPS Guest
                                                // Support ; #001: BAS Game
                                                // Stability: TU8: The game
                                                // crashes when second Guest
                                                // signs-in on console which
                                                // takes part in Xbox LIVE
                                                // multiplayer session.
                                                // PlatformProfile.RequestConvertOfflineToGuestUI(
                                                // &Minecraft::InGame_SignInReturned,
                                                // this,i);

                                                ui.HidePressStart();
                                                {
                                                    uint32_t uiIDA[1];
                                                    uiIDA[0] = IDS_CONFIRM_OK;
                                                    ui.RequestErrorMessage(
                                                        IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                                                        IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT,
                                                        uiIDA, 1, i);
                                                }
                                            }
                                            // else
                                            {
                                                // player not signed in to live
                                                // bring up the sign in dialog
                                                Log::info(
                                                    "Bringing up the sign in "
                                                    "ui\n");
                                                PlatformProfile.RequestSignInUI(
                                                    false,
                                                    NetworkService
                                                        .IsLocalGame(),
                                                    true, false, true,
                                                    [this](bool b, int p) {
                                                        return InGame_SignInReturned(
                                                            this, b, p);
                                                    },
                                                    i);
                                            }
                                        }
                                    } else {
                                        // bring up the sign in dialog
                                        Log::info(
                                            "Bringing up the sign in ui\n");
                                        PlatformProfile.RequestSignInUI(
                                            false, NetworkService.IsLocalGame(),
                                            true, false, true,
                                            [this](bool b, int p) {
                                                return InGame_SignInReturned(
                                                    this, b, p);
                                            },
                                            i);
                                    }
                                }
                            }
                        }
                    }
                }

                if (pause && level != nullptr) {
                    float lastA = timer->a;
                    timer->advanceTime();
                    timer->a = lastA;
                } else {
                    timer->advanceTime();
                }

                // int64_t beforeTickTime = System::nanoTime();
                for (int i = 0; i < timer->ticks; i++) {
                    bool bLastTimerTick = (i == (timer->ticks - 1));
                    // 4J-PB - the tick here can run more than once, and this is
                    // a problem for our input, which would see the a key press
                    // twice with the same time - let's tick the inputmanager
                    // again
                    if (i != 0) {
                        PlatformInput.Tick();
                        gameServices().handleButtonPresses();
                    }

                    ticks++;
                    //            try {		// 4J - try/catch removed
                    bool bFirst = true;
                    for (int idx = 0; idx < XUSER_MAX_COUNT; idx++) {
                        // 4J - If we are waiting for this connection to do
                        // something, then tick it here. This replaces many of
                        // the original Java scenes which would tick the
                        // connection while showing that scene
                        if (m_pendingLocalConnections[idx] != nullptr) {
                            m_pendingLocalConnections[idx]->tick();
                        }

                        // reset the player inactive tick
                        if (localplayers[idx] != nullptr) {
                            // any input received?
                            if ((localplayers[idx]->ullButtonsPressed != 0) ||
                                PlatformInput.GetJoypadStick_LX(idx, false) !=
                                    0.0f ||
                                PlatformInput.GetJoypadStick_LY(idx, false) !=
                                    0.0f ||
                                PlatformInput.GetJoypadStick_RX(idx, false) !=
                                    0.0f ||
                                PlatformInput.GetJoypadStick_RY(idx, false) !=
                                    0.0f) {
                                localplayers[idx]->ResetInactiveTicks();
                            } else {
                                localplayers[idx]->IncrementInactiveTicks();
                            }

                            if (localplayers[idx]->GetInactiveTicks() > 200) {
                                if (!localplayers[idx]->isIdle() &&
                                    localplayers[idx]->onGround) {
                                    localplayers[idx]->setIsIdle(true);
                                }
                            } else {
                                if (localplayers[idx]->isIdle()) {
                                    localplayers[idx]->setIsIdle(false);
                                }
                            }
                        }

                        if (setLocalPlayerIdx(idx)) {
                            tick(bFirst, bLastTimerTick);
                            bFirst = false;
                            // clear the stored button downs since the tick for
                            // this player will now have actioned them
                            player->ullButtonsPressed = 0LL;
                        } else if (screen != nullptr) {
                            screen->updateEvents();
                            // 4jcraft: this fixes the title screen panorama
                            // running faster than it should
                            if (!idx) {
                                screen->tick();
                            }
                        }
                    }

                    ui.HandleGameTick();

                    setLocalPlayerIdx(PlatformInput.GetPrimaryPad());

                    // 4J - added - now do the equivalent of level::animateTick,
                    // but taking into account the positions of all our players

                    for (int l = 0; l < levels.size(); l++) {
                        if (levels[l]) {
                            levels[l]->animateTickDoWork();
                        }
                    }

                    //            } catch (LevelConflictException e) {
                    //                this.level = null;
                    //                setLevel(null);
                    //                setScreen(new LevelConflictScreen());
                    //            }
                    // 				SparseLightStorage::tick();
                    // // 4J added
                    // CompressedTileStorage::tick();	// 4J added
                    // 				SparseDataStorage::tick();
                    // // 4J added
                }
                // int64_t tickDuraction = System::nanoTime() - beforeTickTime;
                checkGlError("Pre render");

                TileRenderer::fancy = options->fancyGraphics;

                // if (pause) timer.a = 1;

                soundEngine->tick((std::shared_ptr<Mob>*)localplayers,
                                  timer->a);

                // if (level != nullptr) level->updateLights();
                glEnable(GL_TEXTURE_2D);

                //        if (!Keyboard::isKeyDown(Keyboard.KEY_F7))
                //        Display.update();		// 4J - removed

                // 4J-PB - changing this to be per player
                // if (player != nullptr && player->isInWall())
                // options->thirdPersonView = false;
                if (player != nullptr && player->isInWall())
                    player->SetThirdPersonView(0);

                if (!noRender) {
                    bool bFirst = true;
                    int iPrimaryPad = PlatformInput.GetPrimaryPad();
                    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                        if (setLocalPlayerIdx(i)) {
                            PlatformRenderer.StateSetViewport(
                                (IPlatformRenderer::eViewportType)
                                    player->m_iScreenSection);
                            gameRenderer->render(timer->a, bFirst);
                            bFirst = false;

                            if (i == iPrimaryPad) {
                                // check to see if we need to capture a
                                // screenshot for the save game thumbnail
                                switch (gameServices().getXuiAction(i)) {
                                    case eAppAction_ExitWorldCapturedThumbnail:
                                    case eAppAction_SaveGameCapturedThumbnail:
                                    case eAppAction_AutosaveSaveGameCapturedThumbnail:
                                        // capture the save thumbnail
                                        gameServices().captureSaveThumbnail();
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                    }

#if !defined(_ENABLEIGGY)
                    // On Linux, Iggy Flash UI is not available. If no players
                    // were rendered (menu / title-screen state), call
                    // GameRenderer directly so mc->screen draws.
                    if (bFirst) {
                        localPlayerIdx = 0;
                        PlatformRenderer.StateSetViewport(
                            IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN);
                        gameRenderer->render(timer->a, true);
                    }
#endif

                    // If there's an unoccupied quadrant, then clear that to
                    // black
                    if (unoccupiedQuadrant > -1) {
                        // render a logo
                        PlatformRenderer.StateSetViewport((
                            IPlatformRenderer::eViewportType)(
                            IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT +
                            unoccupiedQuadrant));
                        glClearColor(0, 0, 0, 0);
                        glClear(GL_COLOR_BUFFER_BIT);

                        ui.SetEmptyQuadrantLogo(
                            IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT +
                            unoccupiedQuadrant);
                    }
                    setLocalPlayerIdx(iPrimaryPad);
                    PlatformRenderer.StateSetViewport(
                        IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN);
                }
                glFlush();

                /*	4J - removed
                if (!Display::isActive())
                {
                if (fullscreen)
                {
                this->toggleFullScreen();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                */

#if PACKET_ENABLE_STAT_TRACKING
                Packet::updatePacketStatsPIX();
#endif

                if (options->renderDebug) {
                    // renderFpsMeter(tickDuraction);

#if DEBUG_RENDER_SHOWS_PACKETS
                    // To show data for only one packet type
                    // Packet::renderPacketStats(31);

                    // To show data for all packet types selected as being
                    // renderable in the Packet:static_ctor call to Packet::map
                    Packet::renderAllPacketStats();
#else
                    // To show the size of the QNet queue in bytes and messages
                    NetworkService.renderQueueMeter();
#endif
                } else {
                    lastTimer = System::nanoTime();
                }

                achievementPopup->render();

                std::this_thread::yield();  // 4jcraft added now that we have
                                            // portable thread yield.
                                            // std::this_thread::sleep_for(
                //     std::chrono::milliseconds(0));  // 4J - was
                //     Thread.yield())

                //        if (Keyboard::isKeyDown(Keyboard::KEY_F7))
                //        Display.update();	// 4J - removed condition
                Display::update();

                //        checkScreenshot();	// 4J - removed

                /* 4J - removed
                if (parent != nullptr && !fullscreen)
                {
                if (parent.getWidth() != width || parent.getHeight() != height)
                {
                width = parent.getWidth();
                height = parent.getHeight();
                if (width <= 0) width = 1;
                if (height <= 0) height = 1;

                resize(width, height);
                }
                }
                */
                checkGlError("Post render");
                frames++;
                // pause = !isClientSide() && screen != nullptr &&
                // screen->isPauseScreen();
#if defined(ENABLE_JAVA_GUIS)
                pause = NetworkService.IsLocalGame() &&
                        NetworkService.GetPlayerCount() == 1 &&
                        screen != nullptr && screen->isPauseScreen();
#else
                pause = gameServices().isAppPaused();
#endif

#if !defined(_CONTENT_PACKAGE)
                while (System::nanoTime() >= lastTime + 1000000000) {
                    fpsString = toWString<int>(frames) + " fps, " +
                                toWString<int>(Chunk::updates) +
                                " chunk updates";
                    Chunk::updates = 0;
                    lastTime += 1000000000;
                    frames = 0;
                }
#endif
                /*
                } catch (LevelConflictException e) {
                this.level = null;
                setLevel(null);
                setScreen(new LevelConflictScreen());
                } catch (OutOfMemoryError e) {
                emergencySave();
                setScreen(new OutOfMemoryScreen());
                System.gc();
                }
                */
            }
            /*
            } catch (StopGameException e) {
            } catch (Throwable e) {
            emergencySave();
            e.printStackTrace();
            crash(new CrashReport("Unexpected error", e));
            } finally {
            destroy();
            }
            */
        }
    }  // lock_guard scope
}

void Minecraft::run_end() { destroy(); }

void Minecraft::emergencySave() {
    // 4J - lots of try/catches removed here, and garbage collector things
    levelRenderer->clear();
    setLevel(nullptr);
}

void Minecraft::renderFpsMeter(int64_t tickTime) {
    int nsPer60Fps = 1000000000l / 60;
    if (lastTimer == -1) {
        lastTimer = System::nanoTime();
    }
    int64_t now = System::nanoTime();
    Minecraft::tickTimes[(Minecraft::frameTimePos) &
                         (Minecraft::frameTimes_length - 1)] = tickTime;
    Minecraft::frameTimes[(Minecraft::frameTimePos++) &
                          (Minecraft::frameTimes_length - 1)] = now - lastTimer;
    lastTimer = now;

    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glEnable(GL_COLOR_MATERIAL);
    glLoadIdentity();
    glOrtho(0, (float)width, (float)height, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2000);

    glLineWidth(1);
    glDisable(GL_TEXTURE_2D);
    Tesselator* t = Tesselator::getInstance();
    t->begin(GL_QUADS);
    int hh1 = (int)(nsPer60Fps / 200000);
    t->color(0x20000000);
    t->vertex((float)(0), (float)(height - hh1), (float)(0));
    t->vertex((float)(0), (float)(height), (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height),
              (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height - hh1),
              (float)(0));

    t->color(0x20200000);
    t->vertex((float)(0), (float)(height - hh1 * 2), (float)(0));
    t->vertex((float)(0), (float)(height - hh1), (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height - hh1),
              (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height - hh1 * 2),
              (float)(0));

    t->end();
    int64_t totalTime = 0;
    for (int i = 0; i < Minecraft::frameTimes_length; i++) {
        totalTime += Minecraft::frameTimes[i];
    }
    int hh = (int)(totalTime / 200000 / Minecraft::frameTimes_length);
    t->begin(GL_QUADS);
    t->color(0x20400000);
    t->vertex((float)(0), (float)(height - hh), (float)(0));
    t->vertex((float)(0), (float)(height), (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height),
              (float)(0));
    t->vertex((float)(Minecraft::frameTimes_length), (float)(height - hh),
              (float)(0));
    t->end();
    t->begin(GL_LINES);
    for (int i = 0; i < Minecraft::frameTimes_length; i++) {
        int col = ((i - Minecraft::frameTimePos) &
                   (Minecraft::frameTimes_length - 1)) *
                  255 / Minecraft::frameTimes_length;
        int cc = col * col / 255;
        cc = cc * cc / 255;
        int cc2 = cc * cc / 255;
        cc2 = cc2 * cc2 / 255;
        if (Minecraft::frameTimes[i] > nsPer60Fps) {
            t->color(0xff000000 + cc * 65536);
        } else {
            t->color(0xff000000 + cc * 256);
        }

        int64_t time = Minecraft::frameTimes[i] / 200000;
        int64_t time2 = Minecraft::tickTimes[i] / 200000;

        t->vertex((float)(i + 0.5f), (float)(height - time + 0.5f), (float)(0));
        t->vertex((float)(i + 0.5f), (float)(height + 0.5f), (float)(0));

        // if (Minecraft.frameTimes[i]>nsPer60Fps) {
        t->color(0xff000000 + cc * 65536 + cc * 256 + cc * 1);
        // } else {
        // t.color(0xff808080 + cc/2 * 256);
        // }
        t->vertex((float)(i + 0.5f), (float)(height - time + 0.5f), (float)(0));
        t->vertex((float)(i + 0.5f), (float)(height - (time - time2) + 0.5f),
                  (float)(0));
    }
    t->end();

    glEnable(GL_TEXTURE_2D);
}

void Minecraft::stop() {
    running = false;
    // keepPolling = false;
}

void Minecraft::pauseGame() {
    if (screen != nullptr) {
        // 4jcraft: Pass the keypress to the screen
        // normally this would've been done in updateEvents(), but it works
        // better here (for now atleast)
        screen->keyPressed(0, Keyboard::KEY_ESCAPE);
        return;
    }
#if defined(ENABLE_JAVA_GUIS)
    setScreen(new PauseScreen());  // 4J - TODO put back in
#endif
}

bool Minecraft::pollResize() {
    int fbw, fbh;
    PlatformRenderer.GetFramebufferSize(fbw, fbh);
    if (fbw != width_phys || fbh != height_phys) {
        resize(fbw, fbh);
        return true;
    }
    return false;
}

void Minecraft::resize(int width, int height) {
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    // 4jcraft: store physical framebuffer size and adjust logical width
    // for non-widescreen aspect ratio to fix UI scaling.
    this->width_phys = width;
    this->height_phys = height;
    if (PlatformRenderer.IsWidescreen()) {
        this->width = width;
    } else {
        this->width = (width * 3) / 4;
    }
    this->height = height;

    if (screen != nullptr) {
        // 4jcraft: use adjusted logical width instead of raw width for correct
        // screen size calculation.
        ScreenSizeCalculator ssc(options, this->width, height);
        int screenWidth = ssc.getWidth();
        int screenHeight = ssc.getHeight();
        screen->init(
            this, screenWidth,
            screenHeight);  // 4jcraft: uncommented to immediately scale on
                            // resize now that we have correct ssc usage
    }
}

void Minecraft::verify() {
    /* 4J - TODO
    new Thread() {
    public void run() {
    try {
    HttpURLConnection huc = (HttpURLConnection) new
    URL("https://login.minecraft.net/session?name=" + user.name + "&session=" +
    user.sessionId).openConnection(); huc.connect(); if (huc.getResponseCode()
    == 400) { warezTime = System.currentTimeMillis();
    }
    huc.disconnect();
    } catch (Exception e) {
    e.printStackTrace();
    }
    }
    }.start();
    */
}

void Minecraft::levelTickUpdateFunc(void* pParam) {
    Level* pLevel = (Level*)pParam;
    pLevel->tick();
}

void Minecraft::levelTickThreadInitFunc() {
    Compression::UseDefaultThreadStorage();
}

// 4J - added bFirst parameter, which is true for the first active viewport in
// splitscreen 4J - added bUpdateTextures, which is true if the actual renderer
// textures are to be updated - this will be true for the last time this tick
// runs with bFirst true
void Minecraft::tick(bool bFirst, bool bUpdateTextures) {
    int iPad = player->GetXboxPad();
    // OutputDebugString("Minecraft::tick\n");

    // 4J-PB - only tick this player's stats
    stats[iPad]->tick(iPad);

    // Tick the opacity timer (to display the interface at default opacity for a
    // certain time if the user has been navigating it)
    gameServices().tickOpacityTimer(iPad);

    // 4J added
    if (bFirst) levelRenderer->destroyedTileManager->tick();

    gui->tick();
    gameRenderer->pick(1);

    // soundEngine.playMusicTick();

    if (!pause && level != nullptr) gameMode->tick();
    glBindTexture(GL_TEXTURE_2D,
                  textures->loadTexture(TN_TERRAIN));  // "/terrain.png"));
    if (bFirst) {
        if (!pause) textures->tick(bUpdateTextures);
    }

    /*
     * if (serverConnection != null && !(screen instanceof ErrorScreen)) {
     * if (!serverConnection.isConnected()) {
     * progressRenderer.progressStart("Connecting..");
     * progressRenderer.progressStagePercentage(0); } else {
     * serverConnection.tick(); serverConnection.sendPosition(player); } }
     */
    if (screen == nullptr && player != nullptr) {
        if (player->getHealth() <= 0 && !ui.GetMenuDisplayed(iPad)) {
            setScreen(nullptr);
        } else if (player->isSleeping() && level != nullptr &&
                   level->isClientSide) {
            //            setScreen(new InBedChatScreen());		// 4J -
            //            TODO put back in
        }
    } else if (screen != nullptr &&
               (dynamic_cast<InBedChatScreen*>(screen) != nullptr) &&
               !player->isSleeping()) {
        setScreen(nullptr);
    }

    if (screen != nullptr) {
        player->missTime = 10000;
        player->lastClickTick[0] = ticks + 10000;
        player->lastClickTick[1] = ticks + 10000;
    }

    if (screen != nullptr) {
        screen->updateEvents();
        if (screen != nullptr) {
            screen->particles->tick();
            screen->tick();
        }
    }

    if (screen == nullptr && !ui.GetMenuDisplayed(iPad)) {
        // 4J-PB - add some tooltips if required
        int iA = -1, iB = -1, iX, iY = IDS_CONTROLS_INVENTORY, iLT = -1,
            iRT = -1, iLB = -1, iRB = -1, iLS = -1, iRS = -1;

        if (player->abilities.instabuild) {
            iX = IDS_TOOLTIPS_CREATIVE;
        } else {
            iX = IDS_CONTROLS_CRAFTING;
        }
        // control scheme remapping can move the Action button, so we need to
        // check this
        int* piAction;
        int* piJump;
        int* piUse;
        int* piAlt;

        unsigned int uiAction = PlatformInput.GetGameJoypadMaps(
            PlatformInput.GetJoypadMapVal(iPad), MINECRAFT_ACTION_ACTION);
        unsigned int uiJump = PlatformInput.GetGameJoypadMaps(
            PlatformInput.GetJoypadMapVal(iPad), MINECRAFT_ACTION_JUMP);
        unsigned int uiUse = PlatformInput.GetGameJoypadMaps(
            PlatformInput.GetJoypadMapVal(iPad), MINECRAFT_ACTION_USE);
        unsigned int uiAlt = PlatformInput.GetGameJoypadMaps(
            PlatformInput.GetJoypadMapVal(iPad), MINECRAFT_ACTION_SNEAK_TOGGLE);

        // Also need to handle PS3 having swapped triggers/bumpers
        switch (uiAction) {
            case _360_JOY_BUTTON_RT:
                piAction = &iRT;
                break;
            case _360_JOY_BUTTON_LT:
                piAction = &iLT;
                break;
            case _360_JOY_BUTTON_LB:
                piAction = &iLB;
                break;
            case _360_JOY_BUTTON_RB:
                piAction = &iRB;
                break;
            case _360_JOY_BUTTON_A:
            default:
                piAction = &iA;
                break;
        }

        switch (uiJump) {
            case _360_JOY_BUTTON_LT:
                piJump = &iLT;
                break;
            case _360_JOY_BUTTON_RT:
                piJump = &iRT;
                break;
            case _360_JOY_BUTTON_LB:
                piJump = &iLB;
                break;
            case _360_JOY_BUTTON_RB:
                piJump = &iRB;
                break;
            case _360_JOY_BUTTON_A:
            default:
                piJump = &iA;
                break;
        }

        switch (uiUse) {
            case _360_JOY_BUTTON_LB:
                piUse = &iLB;
                break;
            case _360_JOY_BUTTON_RB:
                piUse = &iRB;
                break;
            case _360_JOY_BUTTON_LT:
                piUse = &iLT;
                break;
            case _360_JOY_BUTTON_RT:
            default:
                piUse = &iRT;
                break;
        }

        switch (uiAlt) {
            default:
            case _360_JOY_BUTTON_LSTICK_RIGHT:
                piAlt = &iRS;
                break;

                // TODO
        }

        if (player->isUnderLiquid(Material::water)) {
            *piJump = IDS_TOOLTIPS_SWIMUP;
        } else {
            *piJump = -1;
        }

        *piUse = -1;
        *piAction = -1;
        *piAlt = -1;

        // 4J-PB another special case for when the player is sleeping in a bed
        if (player->isSleeping() && (level != nullptr) && level->isClientSide) {
            *piUse = IDS_TOOLTIPS_WAKEUP;
        } else {
            if (player->isRiding()) {
                std::shared_ptr<Entity> mount = player->riding;

                if (mount->instanceof (eTYPE_MINECART) || mount->instanceof
                    (eTYPE_BOAT)) {
                    *piAlt = IDS_TOOLTIPS_EXIT;
                } else {
                    *piAlt = IDS_TOOLTIPS_DISMOUNT;
                }
            }

            // no hit result, but we may have something in our hand that we can
            // do something with
            std::shared_ptr<ItemInstance> itemInstance =
                player->inventory->getSelected();

            // 4J-JEV: Moved all this here to avoid having it in 3 different
            // places.
            if (itemInstance) {
                // 4J-PB - very special case for boat and empty bucket and glass
                // bottle and more
                bool bUseItem =
                    gameMode->useItem(player, level, itemInstance, true);

                switch (itemInstance->getItem()->id) {
                        // food
                    case Item::potatoBaked_Id:
                    case Item::potato_Id:
                    case Item::pumpkinPie_Id:
                    case Item::potatoPoisonous_Id:
                    case Item::carrotGolden_Id:
                    case Item::carrots_Id:
                    case Item::mushroomStew_Id:
                    case Item::apple_Id:
                    case Item::bread_Id:
                    case Item::porkChop_raw_Id:
                    case Item::porkChop_cooked_Id:
                    case Item::apple_gold_Id:
                    case Item::fish_raw_Id:
                    case Item::fish_cooked_Id:
                    case Item::cookie_Id:
                    case Item::beef_cooked_Id:
                    case Item::beef_raw_Id:
                    case Item::chicken_cooked_Id:
                    case Item::chicken_raw_Id:
                    case Item::melon_Id:
                    case Item::rotten_flesh_Id:
                    case Item::spiderEye_Id:
                        // Check that we are actually hungry so will eat this
                        // item
                        {
                            FoodItem* food = (FoodItem*)itemInstance->getItem();
                            if (food != nullptr && food->canEat(player)) {
                                *piUse = IDS_TOOLTIPS_EAT;
                            }
                        }
                        break;

                    case Item::bucket_milk_Id:
                        *piUse = IDS_TOOLTIPS_DRINK;
                        break;

                    case Item::fishingRod_Id:  // use
                    case Item::emptyMap_Id:
                        *piUse = IDS_TOOLTIPS_USE;
                        break;

                    case Item::egg_Id:  // throw
                    case Item::snowBall_Id:
                        *piUse = IDS_TOOLTIPS_THROW;
                        break;

                    case Item::bow_Id:  // draw or release
                        if (player->abilities.instabuild ||
                            player->inventory->hasResource(Item::arrow_Id)) {
                            if (player->isUsingItem())
                                *piUse = IDS_TOOLTIPS_RELEASE_BOW;
                            else
                                *piUse = IDS_TOOLTIPS_DRAW_BOW;
                        }
                        break;

                    case Item::sword_wood_Id:
                    case Item::sword_stone_Id:
                    case Item::sword_iron_Id:
                    case Item::sword_diamond_Id:
                    case Item::sword_gold_Id:
                        *piUse = IDS_TOOLTIPS_BLOCK;
                        break;

                    case Item::bucket_empty_Id:
                    case Item::glassBottle_Id:
                        if (bUseItem) *piUse = IDS_TOOLTIPS_COLLECT;
                        break;

                    case Item::bucket_lava_Id:
                    case Item::bucket_water_Id:
                        *piUse = IDS_TOOLTIPS_EMPTY;
                        break;

                    case Item::boat_Id:
                    case Tile::waterLily_Id:
                        if (bUseItem) *piUse = IDS_TOOLTIPS_PLACE;
                        break;

                    case Item::potion_Id:
                        if (bUseItem) {
                            if (MACRO_POTION_IS_SPLASH(
                                    itemInstance->getAuxValue()))
                                *piUse = IDS_TOOLTIPS_THROW;
                            else
                                *piUse = IDS_TOOLTIPS_DRINK;
                        }
                        break;

                    case Item::enderPearl_Id:
                        if (bUseItem) *piUse = IDS_TOOLTIPS_THROW;
                        break;

                    case Item::eyeOfEnder_Id:
                        // This will only work if there is a stronghold in this
                        // dimension
                        if (bUseItem && (level->dimension->id == 0) &&
                            level->getLevelData()->getHasStronghold()) {
                            *piUse = IDS_TOOLTIPS_THROW;
                        }
                        break;

                    case Item::expBottle_Id:
                        if (bUseItem) *piUse = IDS_TOOLTIPS_THROW;
                        break;
                }
            }

            if (hitResult != nullptr) {
                switch (hitResult->type) {
                    case HitResult::TILE: {
                        int x, y, z;
                        x = hitResult->x;
                        y = hitResult->y;
                        z = hitResult->z;
                        int face = hitResult->f;

                        int iTileID = level->getTile(x, y, z);
                        int iData = level->getData(x, y, z);

                        if (gameMode != nullptr &&
                            gameMode->getTutorial() != nullptr) {
                            // 4J Stu - For the tutorial we want to be able to
                            // record what items we look at so that we can give
                            // hints
                            gameMode->getTutorial()->onLookAt(iTileID, iData);
                        }

                        // 4J-PB - Call the useItemOn with the TestOnly flag set
                        bool bUseItemOn = gameMode->useItemOn(
                            player, level, itemInstance, x, y, z, face,
                            &hitResult->pos, true);

                        /* 4J-Jev:
                         *	Moved this here so we have item tooltips to
                         * fallback on for noteblocks, enderportals and
                         * flowerpots in case of non-standard items. (ie. ignite
                         * behaviour)
                         */
                        if (bUseItemOn && itemInstance != nullptr) {
                            switch (itemInstance->getItem()->id) {
                                case Tile::mushroom_brown_Id:
                                case Tile::mushroom_red_Id:
                                case Tile::tallgrass_Id:
                                case Tile::cactus_Id:
                                case Tile::sapling_Id:
                                case Tile::reeds_Id:
                                case Tile::flower_Id:
                                case Tile::rose_Id:
                                    *piUse = IDS_TOOLTIPS_PLANT;
                                    break;

                                    // Things to USE
                                case Item::hoe_wood_Id:
                                case Item::hoe_stone_Id:
                                case Item::hoe_iron_Id:
                                case Item::hoe_diamond_Id:
                                case Item::hoe_gold_Id:
                                    *piUse = IDS_TOOLTIPS_TILL;
                                    break;

                                case Item::seeds_wheat_Id:
                                case Item::netherwart_seeds_Id:
                                    *piUse = IDS_TOOLTIPS_PLANT;
                                    break;

                                case Item::dye_powder_Id:
                                    // bonemeal grows various plants
                                    if (itemInstance->getAuxValue() ==
                                        DyePowderItem::WHITE) {
                                        switch (iTileID) {
                                            case Tile::sapling_Id:
                                            case Tile::wheat_Id:
                                            case Tile::grass_Id:
                                            case Tile::mushroom_brown_Id:
                                            case Tile::mushroom_red_Id:
                                            case Tile::melonStem_Id:
                                            case Tile::pumpkinStem_Id:
                                            case Tile::carrots_Id:
                                            case Tile::potatoes_Id:
                                                *piUse = IDS_TOOLTIPS_GROW;
                                                break;
                                        }
                                    }
                                    break;

                                case Item::painting_Id:
                                    *piUse = IDS_TOOLTIPS_HANG;
                                    break;

                                case Item::flintAndSteel_Id:
                                case Item::fireball_Id:
                                    *piUse = IDS_TOOLTIPS_IGNITE;
                                    break;

                                case Item::fireworks_Id:
                                    *piUse = IDS_TOOLTIPS_FIREWORK_LAUNCH;
                                    break;

                                case Item::lead_Id:
                                    *piUse = IDS_TOOLTIPS_ATTACH;
                                    break;

                                default:
                                    *piUse = IDS_TOOLTIPS_PLACE;
                                    break;
                            }
                        }

                        switch (iTileID) {
                            case Tile::anvil_Id:
                            case Tile::enchantTable_Id:
                            case Tile::brewingStand_Id:
                            case Tile::workBench_Id:
                            case Tile::furnace_Id:
                            case Tile::furnace_lit_Id:
                            case Tile::door_wood_Id:
                            case Tile::dispenser_Id:
                            case Tile::lever_Id:
                            case Tile::button_stone_Id:
                            case Tile::button_wood_Id:
                            case Tile::trapdoor_Id:
                            case Tile::fenceGate_Id:
                            case Tile::beacon_Id:
                                *piAction = IDS_TOOLTIPS_MINE;
                                *piUse = IDS_TOOLTIPS_USE;
                                break;

                            case Tile::chest_Id:
                                *piAction = IDS_TOOLTIPS_MINE;
                                *piUse = (Tile::chest->getContainer(
                                              level, x, y, z) != nullptr)
                                             ? IDS_TOOLTIPS_OPEN
                                             : -1;
                                break;

                            case Tile::enderChest_Id:
                            case Tile::chest_trap_Id:
                            case Tile::dropper_Id:
                            case Tile::hopper_Id:
                                *piUse = IDS_TOOLTIPS_OPEN;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::activatorRail_Id:
                            case Tile::goldenRail_Id:
                            case Tile::detectorRail_Id:
                            case Tile::rail_Id:
                                if (bUseItemOn) *piUse = IDS_TOOLTIPS_PLACE;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::bed_Id:
                                if (bUseItemOn) *piUse = IDS_TOOLTIPS_SLEEP;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::noteblock_Id:
                                // if in creative mode, we will mine
                                if (player->abilities.instabuild)
                                    *piAction = IDS_TOOLTIPS_MINE;
                                else
                                    *piAction = IDS_TOOLTIPS_PLAY;
                                *piUse = IDS_TOOLTIPS_CHANGEPITCH;
                                break;

                            case Tile::sign_Id:
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::cauldron_Id:
                                // special case for a cauldron of water and an
                                // empty bottle
                                if (itemInstance) {
                                    int iID = itemInstance->getItem()->id;
                                    int currentData = level->getData(x, y, z);
                                    if ((iID == Item::glassBottle_Id) &&
                                        (currentData > 0)) {
                                        *piUse = IDS_TOOLTIPS_COLLECT;
                                    }
                                }
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::cake_Id:
                                if (player->abilities
                                        .instabuild)  // if in creative mode, we
                                                      // will mine
                                {
                                    *piAction = IDS_TOOLTIPS_MINE;
                                } else {
                                    if (player->getFoodData()
                                            ->needsFood())  // 4J-JEV: Changed
                                                            // from healthto
                                                            // hunger.
                                    {
                                        *piAction = IDS_TOOLTIPS_EAT;
                                        *piUse = IDS_TOOLTIPS_EAT;
                                    } else {
                                        *piAction = IDS_TOOLTIPS_MINE;
                                    }
                                }
                                break;

                            case Tile::jukebox_Id:
                                if (!bUseItemOn && itemInstance != nullptr) {
                                    int iID = itemInstance->getItem()->id;
                                    if ((iID >= Item::record_01_Id) &&
                                        (iID <= Item::record_12_Id)) {
                                        *piUse = IDS_TOOLTIPS_PLAY;
                                    }
                                    *piAction = IDS_TOOLTIPS_MINE;
                                } else {
                                    if (Tile::jukebox->TestUse(
                                            level, x, y, z,
                                            player))  // means we can eject
                                    {
                                        *piUse = IDS_TOOLTIPS_EJECT;
                                    }
                                    *piAction = IDS_TOOLTIPS_MINE;
                                }
                                break;

                            case Tile::flowerPot_Id:
                                if (!bUseItemOn && (itemInstance != nullptr) &&
                                    (iData == 0)) {
                                    int iID = itemInstance->getItem()->id;
                                    if (iID < 256)  // is it a tile?
                                    {
                                        switch (iID) {
                                            case Tile::flower_Id:
                                            case Tile::rose_Id:
                                            case Tile::sapling_Id:
                                            case Tile::mushroom_brown_Id:
                                            case Tile::mushroom_red_Id:
                                            case Tile::cactus_Id:
                                            case Tile::deadBush_Id:
                                                *piUse = IDS_TOOLTIPS_PLANT;
                                                break;

                                            case Tile::tallgrass_Id:
                                                if (itemInstance
                                                        ->getAuxValue() !=
                                                    TallGrass::TALL_GRASS)
                                                    *piUse = IDS_TOOLTIPS_PLANT;
                                                break;
                                        }
                                    }
                                }
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::comparator_off_Id:
                            case Tile::comparator_on_Id:
                                *piUse = IDS_TOOLTIPS_USE;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::diode_off_Id:
                            case Tile::diode_on_Id:
                                *piUse = IDS_TOOLTIPS_USE;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::redStoneOre_Id:
                                if (bUseItemOn) *piUse = IDS_TOOLTIPS_USE;
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            case Tile::door_iron_Id:
                                if (*piUse == IDS_TOOLTIPS_PLACE) {
                                    *piUse = -1;
                                }
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;

                            default:
                                *piAction = IDS_TOOLTIPS_MINE;
                                break;
                        }
                    } break;

                    case HitResult::ENTITY:
                        eINSTANCEOF entityType = hitResult->entity->GetType();

                        if ((gameMode != nullptr) &&
                            (gameMode->getTutorial() != nullptr)) {
                            // 4J Stu - For the tutorial we want to be able to
                            // record what items we look at so that we can give
                            // hints
                            gameMode->getTutorial()->onLookAtEntity(
                                hitResult->entity);
                        }

                        std::shared_ptr<ItemInstance> heldItem = nullptr;
                        if (player->inventory->IsHeldItem()) {
                            heldItem = player->inventory->getSelected();
                        }
                        int heldItemId =
                            heldItem != nullptr ? heldItem->getItem()->id : -1;

                        switch (entityType) {
                            case eTYPE_CHICKEN: {
                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                std::shared_ptr<Animal> animal =
                                    std::dynamic_pointer_cast<Animal>(
                                        hitResult->entity);

                                if (animal->isLeashed() &&
                                    animal->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                    break;
                                }

                                switch (heldItemId) {
                                    case Item::nameTag_Id:
                                        *piUse = IDS_TOOLTIPS_NAME;
                                        break;

                                    case Item::lead_Id:
                                        if (!animal->isLeashed())
                                            *piUse = IDS_TOOLTIPS_LEASH;
                                        break;

                                    default: {
                                        if (!animal->isBaby() &&
                                            !animal->isInLove() &&
                                            (animal->getAge() == 0) &&
                                            animal->isFood(heldItem)) {
                                            *piUse = IDS_TOOLTIPS_LOVEMODE;
                                        }
                                    } break;

                                    case -1:
                                        break;  // 4J-JEV: Empty hand.
                                }
                            } break;

                            case eTYPE_COW: {
                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                std::shared_ptr<Animal> animal =
                                    std::dynamic_pointer_cast<Animal>(
                                        hitResult->entity);

                                if (animal->isLeashed() &&
                                    animal->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                    break;
                                }

                                switch (heldItemId) {
                                        // Things to USE
                                    case Item::nameTag_Id:
                                        *piUse = IDS_TOOLTIPS_NAME;
                                        break;
                                    case Item::lead_Id:
                                        if (!animal->isLeashed())
                                            *piUse = IDS_TOOLTIPS_LEASH;
                                        break;
                                    case Item::bucket_empty_Id:
                                        *piUse = IDS_TOOLTIPS_MILK;
                                        break;
                                    default: {
                                        if (!animal->isBaby() &&
                                            !animal->isInLove() &&
                                            (animal->getAge() == 0) &&
                                            animal->isFood(heldItem)) {
                                            *piUse = IDS_TOOLTIPS_LOVEMODE;
                                        }
                                    } break;

                                    case -1:
                                        break;  // 4J-JEV: Empty hand.
                                }
                            } break;
                            case eTYPE_MUSHROOMCOW: {
                                // 4J-PB - Fix for #13081 - No tooltip is
                                // displayed for hitting a cow when you have
                                // nothing in your hand
                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                std::shared_ptr<Animal> animal =
                                    std::dynamic_pointer_cast<Animal>(
                                        hitResult->entity);

                                if (animal->isLeashed() &&
                                    animal->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                    break;
                                }

                                // It's an item
                                switch (heldItemId) {
                                        // Things to USE
                                    case Item::nameTag_Id:
                                        *piUse = IDS_TOOLTIPS_NAME;
                                        break;

                                    case Item::lead_Id:
                                        if (!animal->isLeashed())
                                            *piUse = IDS_TOOLTIPS_LEASH;
                                        break;

                                    case Item::bowl_Id:
                                    case Item::
                                        bucket_empty_Id:  // You can milk a
                                                          // mooshroom with
                                                          // either a bowl
                                                          // (mushroom soup) or
                                                          // a bucket (milk)!
                                        *piUse = IDS_TOOLTIPS_MILK;
                                        break;
                                    case Item::shears_Id: {
                                        if (player->isAllowedToAttackAnimals())
                                            *piAction = IDS_TOOLTIPS_HIT;
                                        if (!animal->isBaby())
                                            *piUse = IDS_TOOLTIPS_SHEAR;
                                    } break;
                                    default: {
                                        if (!animal->isBaby() &&
                                            !animal->isInLove() &&
                                            (animal->getAge() == 0) &&
                                            animal->isFood(heldItem)) {
                                            *piUse = IDS_TOOLTIPS_LOVEMODE;
                                        }
                                    } break;

                                    case -1:
                                        break;  // 4J-JEV: Empty hand.
                                }
                            } break;

                            case eTYPE_BOAT:
                                *piAction = IDS_TOOLTIPS_MINE;
                                *piUse = IDS_TOOLTIPS_SAIL;
                                break;

                            case eTYPE_MINECART_RIDEABLE:
                                *piAction = IDS_TOOLTIPS_MINE;
                                *piUse =
                                    IDS_TOOLTIPS_RIDE;  // are we in the
                                                        // minecart already? -
                                                        // 4J-JEV: Doesn't
                                                        // matter anymore.
                                break;

                            case eTYPE_MINECART_FURNACE:
                                *piAction = IDS_TOOLTIPS_MINE;

                                // if you have coal, it'll go. Is there an
                                // object in hand?
                                if (heldItemId == Item::coal_Id)
                                    *piUse = IDS_TOOLTIPS_USE;
                                break;

                            case eTYPE_MINECART_CHEST:
                            case eTYPE_MINECART_HOPPER:
                                *piAction = IDS_TOOLTIPS_MINE;
                                *piUse = IDS_TOOLTIPS_OPEN;
                                break;

                            case eTYPE_MINECART_SPAWNER:
                            case eTYPE_MINECART_TNT:
                                *piUse = IDS_TOOLTIPS_MINE;
                                break;

                            case eTYPE_SHEEP: {
                                // can dye a sheep
                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                std::shared_ptr<Sheep> sheep =
                                    std::dynamic_pointer_cast<Sheep>(
                                        hitResult->entity);

                                if (sheep->isLeashed() &&
                                    sheep->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                    break;
                                }

                                switch (heldItemId) {
                                    case Item::nameTag_Id:
                                        *piUse = IDS_TOOLTIPS_NAME;
                                        break;

                                    case Item::lead_Id:
                                        if (!sheep->isLeashed())
                                            *piUse = IDS_TOOLTIPS_LEASH;
                                        break;

                                    case Item::dye_powder_Id: {
                                        // convert to tile-based color value (0
                                        // is white instead of black)
                                        int newColor = ColoredTile::
                                            getTileDataForItemAuxValue(
                                                heldItem->getAuxValue());

                                        // can only use a dye on sheep that
                                        // haven't been sheared
                                        if (!(sheep->isSheared() &&
                                              sheep->getColor() != newColor)) {
                                            *piUse = IDS_TOOLTIPS_DYE;
                                        }
                                    } break;
                                    case Item::shears_Id: {
                                        // can only shear a sheep that hasn't
                                        // been sheared
                                        if (!sheep->isBaby() &&
                                            !sheep->isSheared()) {
                                            *piUse = IDS_TOOLTIPS_SHEAR;
                                        }
                                    }

                                    break;
                                    default: {
                                        if (!sheep->isBaby() &&
                                            !sheep->isInLove() &&
                                            (sheep->getAge() == 0) &&
                                            sheep->isFood(heldItem)) {
                                            *piUse = IDS_TOOLTIPS_LOVEMODE;
                                        }
                                    } break;

                                    case -1:
                                        break;  // 4J-JEV: Empty hand.
                                }
                            } break;

                            case eTYPE_PIG: {
                                // can ride a pig
                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                std::shared_ptr<Pig> pig =
                                    std::dynamic_pointer_cast<Pig>(
                                        hitResult->entity);

                                if (pig->isLeashed() &&
                                    pig->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                } else if (heldItemId == Item::lead_Id) {
                                    if (!pig->isLeashed())
                                        *piUse = IDS_TOOLTIPS_LEASH;
                                } else if (heldItemId == Item::nameTag_Id) {
                                    *piUse = IDS_TOOLTIPS_NAME;
                                } else if (pig->hasSaddle())  // does the pig
                                                              // have a saddle?
                                {
                                    *piUse = IDS_TOOLTIPS_MOUNT;
                                } else if (!pig->isBaby()) {
                                    if (player->inventory->IsHeldItem()) {
                                        switch (heldItemId) {
                                            case Item::saddle_Id:
                                                *piUse = IDS_TOOLTIPS_SADDLE;
                                                break;

                                            default: {
                                                if (!pig->isInLove() &&
                                                    (pig->getAge() == 0) &&
                                                    pig->isFood(heldItem)) {
                                                    *piUse =
                                                        IDS_TOOLTIPS_LOVEMODE;
                                                }
                                            } break;
                                        }
                                    }
                                }
                            } break;

                            case eTYPE_WOLF:
                                // can be tamed, fed, and made to sit/stand, or
                                // enter love mode
                                {
                                    std::shared_ptr<Wolf> wolf =
                                        std::dynamic_pointer_cast<Wolf>(
                                            hitResult->entity);

                                    if (player->isAllowedToAttackAnimals())
                                        *piAction = IDS_TOOLTIPS_HIT;

                                    if (wolf->isLeashed() &&
                                        wolf->getLeashHolder() == player) {
                                        *piUse = IDS_TOOLTIPS_UNLEASH;
                                        break;
                                    }

                                    switch (heldItemId) {
                                        case Item::nameTag_Id:
                                            *piUse = IDS_TOOLTIPS_NAME;
                                            break;

                                        case Item::lead_Id:
                                            if (!wolf->isLeashed())
                                                *piUse = IDS_TOOLTIPS_LEASH;
                                            break;

                                        case Item::bone_Id:
                                            if (!wolf->isAngry() &&
                                                !wolf->isTame()) {
                                                *piUse = IDS_TOOLTIPS_TAME;
                                            } else if (
                                                equalsIgnoreCase(
                                                    player->getUUID(),
                                                    wolf->getOwnerUUID())) {
                                                if (wolf->isSitting()) {
                                                    *piUse =
                                                        IDS_TOOLTIPS_FOLLOWME;
                                                } else {
                                                    *piUse = IDS_TOOLTIPS_SIT;
                                                }
                                            }

                                            break;
                                        case Item::enderPearl_Id:
                                            // Use is throw, so don't change the
                                            // tips for the wolf
                                            break;
                                        case Item::dye_powder_Id:
                                            if (wolf->isTame()) {
                                                if (ColoredTile::
                                                        getTileDataForItemAuxValue(
                                                            heldItem
                                                                ->getAuxValue()) !=
                                                    wolf->getCollarColor()) {
                                                    *piUse =
                                                        IDS_TOOLTIPS_DYECOLLAR;
                                                } else if (wolf->isSitting()) {
                                                    *piUse =
                                                        IDS_TOOLTIPS_FOLLOWME;
                                                } else {
                                                    *piUse = IDS_TOOLTIPS_SIT;
                                                }
                                            }
                                            break;
                                        default:
                                            if (wolf->isTame()) {
                                                if (wolf->isFood(heldItem)) {
                                                    if (wolf->GetSynchedHealth() <
                                                        wolf->getMaxHealth()) {
                                                        *piUse =
                                                            IDS_TOOLTIPS_HEAL;
                                                    } else {
                                                        if (!wolf->isBaby() &&
                                                            !wolf->isInLove() &&
                                                            (wolf->getAge() ==
                                                             0)) {
                                                            *piUse =
                                                                IDS_TOOLTIPS_LOVEMODE;
                                                        }
                                                    }
                                                    // break out here
                                                    break;
                                                }

                                                if (equalsIgnoreCase(
                                                        player->getUUID(),
                                                        wolf->getOwnerUUID())) {
                                                    if (wolf->isSitting()) {
                                                        *piUse =
                                                            IDS_TOOLTIPS_FOLLOWME;
                                                    } else {
                                                        *piUse =
                                                            IDS_TOOLTIPS_SIT;
                                                    }
                                                }
                                            }
                                            break;
                                    }
                                }
                                break;
                            case eTYPE_OCELOT: {
                                std::shared_ptr<Ocelot> ocelot =
                                    std::dynamic_pointer_cast<Ocelot>(
                                        hitResult->entity);

                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;

                                if (ocelot->isLeashed() &&
                                    ocelot->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                } else if (heldItemId == Item::lead_Id) {
                                    if (!ocelot->isLeashed())
                                        *piUse = IDS_TOOLTIPS_LEASH;
                                } else if (heldItemId == Item::nameTag_Id) {
                                    *piUse = IDS_TOOLTIPS_NAME;
                                } else if (ocelot->isTame()) {
                                    // 4J-PB - if you have a raw fish in your
                                    // hand, you will feed the ocelot rather
                                    // than have it sit/follow
                                    if (ocelot->isFood(heldItem)) {
                                        if (!ocelot->isBaby()) {
                                            if (!ocelot->isInLove()) {
                                                if (ocelot->getAge() == 0) {
                                                    *piUse =
                                                        IDS_TOOLTIPS_LOVEMODE;
                                                }
                                            } else {
                                                *piUse = IDS_TOOLTIPS_FEED;
                                            }
                                        }

                                    } else if (equalsIgnoreCase(
                                                   player->getUUID(),
                                                   ocelot->getOwnerUUID()) &&
                                               !ocelot->isSittingOnTile()) {
                                        if (ocelot->isSitting()) {
                                            *piUse = IDS_TOOLTIPS_FOLLOWME;
                                        } else {
                                            *piUse = IDS_TOOLTIPS_SIT;
                                        }
                                    }
                                } else if (heldItemId >= 0) {
                                    if (ocelot->isFood(heldItem))
                                        *piUse = IDS_TOOLTIPS_TAME;
                                }
                            } break;

                            case eTYPE_PLAYER: {
                                // Fix for #58576 - TU6: Content: Gameplay: Hit
                                // button prompt is available when attacking a
                                // host who has "Invisible" option turned on
                                std::shared_ptr<Player> TargetPlayer =
                                    std::dynamic_pointer_cast<Player>(
                                        hitResult->entity);

                                if (!TargetPlayer
                                         ->hasInvisiblePrivilege())  // This
                                                                     // means
                                                                     // they are
                                                                     // invisible,
                                                                     // not just
                                                                     // that
                                                                     // they
                                                                     // have the
                                                                     // privilege
                                {
                                    if (gameServices().getGameHostOption(
                                            eGameHostOption_PvP) &&
                                        player->isAllowedToAttackPlayers()) {
                                        *piAction = IDS_TOOLTIPS_HIT;
                                    }
                                }
                            } break;

                            case eTYPE_ITEM_FRAME: {
                                std::shared_ptr<ItemFrame> itemFrame =
                                    std::dynamic_pointer_cast<ItemFrame>(
                                        hitResult->entity);

                                // is the frame occupied?
                                if (itemFrame->getItem() != nullptr) {
                                    // rotate the item
                                    *piUse = IDS_TOOLTIPS_ROTATE;
                                } else {
                                    // is there an object in hand?
                                    if (heldItemId >= 0)
                                        *piUse = IDS_TOOLTIPS_PLACE;
                                }

                                *piAction = IDS_TOOLTIPS_HIT;
                            } break;

                            case eTYPE_VILLAGER: {
                                // 4J-JEV: Cannot leash villagers.

                                std::shared_ptr<Villager> villager =
                                    std::dynamic_pointer_cast<Villager>(
                                        hitResult->entity);
                                if (!villager->isBaby()) {
                                    *piUse = IDS_TOOLTIPS_TRADE;
                                }
                                *piAction = IDS_TOOLTIPS_HIT;
                            } break;

                            case eTYPE_ZOMBIE: {
                                std::shared_ptr<Zombie> zomb =
                                    std::dynamic_pointer_cast<Zombie>(
                                        hitResult->entity);
                                static GoldenAppleItem* goldapple =
                                    (GoldenAppleItem*)Item::apple_gold;

                                // zomb->hasEffect(MobEffect::weakness) - not
                                // present on client.
                                if (zomb->isVillager() && zomb->isWeakened() &&
                                    (heldItemId == Item::apple_gold_Id) &&
                                    !goldapple->isFoil(heldItem)) {
                                    *piUse = IDS_TOOLTIPS_CURE;
                                }
                                *piAction = IDS_TOOLTIPS_HIT;
                            } break;

                            case eTYPE_HORSE: {
                                std::shared_ptr<EntityHorse> horse =
                                    std::dynamic_pointer_cast<EntityHorse>(
                                        hitResult->entity);

                                bool heldItemIsFood = false,
                                     heldItemIsLove = false,
                                     heldItemIsArmour = false;

                                switch (heldItemId) {
                                    case Item::wheat_Id:
                                    case Item::sugar_Id:
                                    case Item::bread_Id:
                                    case Tile::hayBlock_Id:
                                    case Item::apple_Id:
                                        heldItemIsFood = true;
                                        break;
                                    case Item::carrotGolden_Id:
                                    case Item::apple_gold_Id:
                                        heldItemIsLove = true;
                                        heldItemIsFood = true;
                                        break;
                                    case Item::horseArmorDiamond_Id:
                                    case Item::horseArmorGold_Id:
                                    case Item::horseArmorMetal_Id:
                                        heldItemIsArmour = true;
                                        break;
                                }

                                if (horse->isLeashed() &&
                                    horse->getLeashHolder() == player) {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                } else if (heldItemId == Item::lead_Id) {
                                    if (!horse->isLeashed())
                                        *piUse = IDS_TOOLTIPS_LEASH;
                                } else if (heldItemId == Item::nameTag_Id) {
                                    *piUse = IDS_TOOLTIPS_NAME;
                                } else if (horse->isBaby())  // 4J-JEV: Can't
                                                             // ride baby horses
                                                             // due to morals.
                                {
                                    if (heldItemIsFood) {
                                        // 4j - Can feed foles to speed growth.
                                        *piUse = IDS_TOOLTIPS_FEED;
                                    }
                                } else if (!horse->isTamed()) {
                                    if (heldItemId == -1) {
                                        // 4j - Player not holding anything,
                                        // ride and attempt to break untamed
                                        // horse.
                                        *piUse = IDS_TOOLTIPS_TAME;
                                    } else if (heldItemIsFood) {
                                        // 4j - Attempt to make it like you more
                                        // by feeding it.
                                        *piUse = IDS_TOOLTIPS_FEED;
                                    }
                                } else if (player->isSneaking() ||
                                           (heldItemId == Item::saddle_Id) ||
                                           (horse->canWearArmor() &&
                                            heldItemIsArmour)) {
                                    // 4j - Access horses inventory
                                    if (*piUse == -1)
                                        *piUse = IDS_TOOLTIPS_OPEN;
                                } else if (horse->canWearBags() &&
                                           !horse->isChestedHorse() &&
                                           (heldItemId == Tile::chest_Id)) {
                                    // 4j - Attach saddle-bags (chest) to donkey
                                    // or mule.
                                    *piUse = IDS_TOOLTIPS_ATTACH;
                                } else if (horse->isReadyForParenting() &&
                                           heldItemIsLove) {
                                    // 4j - Different food to mate horses.
                                    *piUse = IDS_TOOLTIPS_LOVEMODE;
                                } else if (heldItemIsFood &&
                                           (horse->getHealth() <
                                            horse->getMaxHealth())) {
                                    // 4j - Horse is damaged and can eat held
                                    // item to heal
                                    *piUse = IDS_TOOLTIPS_HEAL;
                                } else {
                                    // 4j - Ride tamed horse.
                                    *piUse = IDS_TOOLTIPS_MOUNT;
                                }

                                if (player->isAllowedToAttackAnimals())
                                    *piAction = IDS_TOOLTIPS_HIT;
                            } break;

                            case eTYPE_ENDERDRAGON:
                                // 4J-JEV: Enderdragon cannot be named.
                                *piAction = IDS_TOOLTIPS_HIT;
                                break;

                            case eTYPE_LEASHFENCEKNOT:
                                *piAction = IDS_TOOLTIPS_UNLEASH;
                                if (heldItemId == Item::lead_Id &&
                                    LeashItem::bindPlayerMobsTest(
                                        player, level, player->x, player->y,
                                        player->z)) {
                                    *piUse = IDS_TOOLTIPS_ATTACH;
                                } else {
                                    *piUse = IDS_TOOLTIPS_UNLEASH;
                                }
                                break;

                            default:
                                if (hitResult->entity->instanceof (eTYPE_MOB)) {
                                    std::shared_ptr<Mob> mob =
                                        std::dynamic_pointer_cast<Mob>(
                                            hitResult->entity);
                                    if (mob->isLeashed() &&
                                        mob->getLeashHolder() == player) {
                                        *piUse = IDS_TOOLTIPS_UNLEASH;
                                    } else if (heldItemId == Item::lead_Id) {
                                        if (!mob->isLeashed())
                                            *piUse = IDS_TOOLTIPS_LEASH;
                                    } else if (heldItemId == Item::nameTag_Id) {
                                        *piUse = IDS_TOOLTIPS_NAME;
                                    }
                                }
                                *piAction = IDS_TOOLTIPS_HIT;
                                break;
                        }
                        break;
                }
            }
        }

        // 4J-JEV: Don't set tooltips when we're reloading the skin, it'll
        // crash.
        if (!ui.IsReloadingSkin())
            ui.SetTooltips(iPad, iA, iB, iX, iY, iLT, iRT, iLB, iRB, iLS, iRS);

        int wheel = 0;
        unsigned int leftTicks =
            PlatformInput.GetValue(iPad, MINECRAFT_ACTION_LEFT_SCROLL, true);
        unsigned int rightTicks =
            PlatformInput.GetValue(iPad, MINECRAFT_ACTION_RIGHT_SCROLL, true);
        if (leftTicks > 0 &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_LEFT_SCROLL)) {
            wheel = (int)leftTicks;  // positive = left
        } else if (rightTicks > 0 &&
                   gameMode->isInputAllowed(MINECRAFT_ACTION_RIGHT_SCROLL)) {
            wheel = -(int)rightTicks;  // negative = right
        }
        if (wheel != 0) {
            player->inventory->swapPaint(wheel);

            if (gameMode != nullptr && gameMode->getTutorial() != nullptr) {
                // 4J Stu - For the tutorial we want to be able to record what
                // items we are using so that we can give hints
                gameMode->getTutorial()->onSelectedItemChanged(
                    player->inventory->getSelected());
            }

            // Update presence
            player->updateRichPresence();

            if (options->isFlying) {
                if (wheel > 0) wheel = 1;
                if (wheel < 0) wheel = -1;

                options->flySpeed += wheel * .25f;
            }
        }

        if (gameMode->isInputAllowed(MINECRAFT_ACTION_ACTION)) {
            if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_ACTION)))
            // if(PlatformInput.ButtonPressed(iPad, MINECRAFT_ACTION_ACTION) )
            {
                // printf("MINECRAFT_ACTION_ACTION ButtonPressed");
                player->handleMouseClick(0);
                player->lastClickTick[0] = ticks;
            }

            if (PlatformInput.ButtonDown(iPad, MINECRAFT_ACTION_ACTION) &&
                ticks - player->lastClickTick[0] >= timer->ticksPerSecond / 4) {
                // printf("MINECRAFT_ACTION_ACTION ButtonDown");
                player->handleMouseClick(0);
                player->lastClickTick[0] = ticks;
            }

            if (PlatformInput.ButtonDown(iPad, MINECRAFT_ACTION_ACTION)) {
                player->handleMouseDown(0, true);
            } else {
                player->handleMouseDown(0, false);
            }
        }

        // 4J Stu - This is how we used to handle the USE action. It has now
        // been replaced with the block below which is more like the way the
        // Java game does it, however we may find that the way we had it
        // previously is more fun to play.
        /*
        if ((PlatformInput.GetValue(iPad, MINECRAFT_ACTION_USE,true)>0) &&
        gameMode->isInputAllowed(MINECRAFT_ACTION_USE) )
        {
        handleMouseClick(1);
        lastClickTick = ticks;
        }
        */
        if (player->isUsingItem()) {
            if (!PlatformInput.ButtonDown(iPad, MINECRAFT_ACTION_USE))
                gameMode->releaseUsingItem(player);
        } else if (gameMode->isInputAllowed(MINECRAFT_ACTION_USE)) {
            if (player->abilities.instabuild) {
                // 4J - attempt to handle click in special creative mode fashion
                // if possible (used for placing blocks at regular intervals)
                bool didClick = player->creativeModeHandleMouseClick(
                    1, PlatformInput.ButtonDown(iPad, MINECRAFT_ACTION_USE));
                // If this handler has put us in lastClick_oldRepeat mode then
                // it is because we aren't placing blocks - behave largely as
                // the code used to
                if (player->lastClickState ==
                    LocalPlayer::lastClick_oldRepeat) {
                    // If we've already handled the click in
                    // creativeModeHandleMouseClick then just record the time of
                    // this click
                    if (didClick) {
                        player->lastClickTick[1] = ticks;
                    } else {
                        // Otherwise just the original game code for handling
                        // autorepeat
                        if (PlatformInput.ButtonDown(iPad,
                                                     MINECRAFT_ACTION_USE) &&
                            ticks - player->lastClickTick[1] >=
                                timer->ticksPerSecond / 4) {
                            player->handleMouseClick(1);
                            player->lastClickTick[1] = ticks;
                        }
                    }
                }
            } else {
                // Consider as a click if we've had a period of not pressing the
                // button, or we've reached auto-repeat time since the last time
                // Auto-repeat is only considered if we aren't riding or
                // sprinting, to avoid photo sensitivity issues when placing
                // fire whilst doing fast things Also disable repeat when the
                // player is sleeping to stop the waking up right after using
                // the bed
                bool firstClick = (player->lastClickTick[1] == 0);
                bool autoRepeat = ticks - player->lastClickTick[1] >=
                                  timer->ticksPerSecond / 4;
                if (player->isRiding() || player->isSprinting() ||
                    player->isSleeping())
                    autoRepeat = false;
                if (PlatformInput.ButtonDown(iPad, MINECRAFT_ACTION_USE)) {
                    // If the player has just exited a bed, then delay the time
                    // before a repeat key is allowed without releasing
                    if (player->isSleeping())
                        player->lastClickTick[1] =
                            ticks + (timer->ticksPerSecond * 2);
                    if (firstClick || autoRepeat) {
                        bool wasSleeping = player->isSleeping();

                        player->handleMouseClick(1);

                        // If the player has just exited a bed, then delay the
                        // time before a repeat key is allowed without releasing
                        if (wasSleeping)
                            player->lastClickTick[1] =
                                ticks + (timer->ticksPerSecond * 2);
                        else
                            player->lastClickTick[1] = ticks;
                    }
                } else {
                    player->lastClickTick[1] = 0;
                }
            }
        }

        if (gameServices().debugSettingsOn()) {
            if (player->ullButtonsPressed &
                (1LL << MINECRAFT_ACTION_CHANGE_SKIN)) {
                player->ChangePlayerSkin();
            }
        }

        if (player->missTime > 0) player->missTime--;

#if defined(_DEBUG_MENUS_ENABLED)
        if (gameServices().debugSettingsOn()) {
            // 4J-PB - debugoverlay for primary player only
            if (iPad == PlatformInput.GetPrimaryPad()) {
                if ((player->ullButtonsPressed &
                     (1LL << MINECRAFT_ACTION_RENDER_DEBUG))) {
#if !defined(_CONTENT_PACKAGE)

                    options->renderDebug = !options->renderDebug;
                    // 4J Stu - The xbox uses a completely different way of
                    // navigating to this scene
                    ui.NavigateToScene(0, eUIScene_DebugOverlay, nullptr,
                                       eUILayer_Debug);
#endif
                }

                if ((player->ullButtonsPressed &
                     (1LL << MINECRAFT_ACTION_SPAWN_CREEPER)) &&
                    gameServices().debugMobsDontAttack()) {
                    // shared_ptr<Mob> mob =
                    // std::dynamic_pointer_cast<Mob>(Creeper::_class->newInstance(
                    // level )); shared_ptr<Mob> mob =
                    // std::dynamic_pointer_cast<Mob>(Wolf::_class->newInstance(
                    // level ));
                    std::shared_ptr<Mob> mob = std::dynamic_pointer_cast<Mob>(
                        std::make_shared<Spider>(level));
                    mob->moveTo(player->x + 1, player->y, player->z + 1,
                                level->random->nextFloat() * 360, 0);
                    level->addEntity(mob);
                }
            }

            if ((player->ullButtonsPressed &
                 (1LL << MINECRAFT_ACTION_FLY_TOGGLE))) {
                player->abilities.debugflying = !player->abilities.debugflying;
                player->abilities.flying = !player->abilities.flying;
            }
        }
#endif

        if ((player->ullButtonsPressed &
             (1LL << MINECRAFT_ACTION_RENDER_THIRD_PERSON)) &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_RENDER_THIRD_PERSON)) {
            // 4J-PB - changing this to be per player
            player->SetThirdPersonView((player->ThirdPersonView() + 1) % 3);
            // options->thirdPersonView = !options->thirdPersonView;
        }

        if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_GAME_INFO)) &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_GAME_INFO)) {
            ui.NavigateToScene(iPad, eUIScene_InGameInfoMenu);
            ui.PlayUISFX(eSFX_Press);
        }

        if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_INVENTORY)) &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_INVENTORY)) {
            std::shared_ptr<MultiplayerLocalPlayer> player =
                Minecraft::GetInstance()->player;
            ui.PlayUISFX(eSFX_Press);
#if defined(ENABLE_JAVA_GUIS)
            setScreen(new InventoryScreen(player));
#else
            gameServices().menus().openInventory(
                iPad, std::static_pointer_cast<LocalPlayer>(player));
#endif
        }

        if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_CRAFTING)) &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_CRAFTING)) {
            std::shared_ptr<MultiplayerLocalPlayer> player =
                Minecraft::GetInstance()->player;

            // 4J-PB - reordered the if statement so creative mode doesn't bring
            // up the crafting table Fix for #39014 - TU5:  Creative Mode:
            // Pressing X to access the creative menu while looking at a
            // crafting table causes the crafting menu to display
            if (gameMode->hasInfiniteItems()) {
                // Creative mode

                ui.PlayUISFX(eSFX_Press);
#if defined(ENABLE_JAVA_GUIS)
                setScreen(new CreativeInventoryScreen(player));
            }
#else
                gameServices().menus().openCreative(
                    iPad, std::static_pointer_cast<LocalPlayer>(player));
            }
            // 4J-PB - Microsoft request that we use the 3x3 crafting if someone
            // presses X while at the workbench
            else if ((hitResult != nullptr) &&
                     (hitResult->type == HitResult::TILE) &&
                     (level->getTile(hitResult->x, hitResult->y,
                                     hitResult->z) == Tile::workBench_Id)) {
                // ui.PlayUISFX(eSFX_Press);
                // app.LoadXuiCrafting3x3Menu(iPad,player,hitResult->x,
                // hitResult->y, hitResult->z);
                bool usedItem = false;
                gameMode->useItemOn(player, level, nullptr, hitResult->x,
                                    hitResult->y, hitResult->z, 0,
                                    &hitResult->pos, false, &usedItem);
            } else {
                ui.PlayUISFX(eSFX_Press);
                gameServices().menus().openCrafting2x2(
                    iPad, std::static_pointer_cast<LocalPlayer>(player));
            }
#endif
        }

        if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_PAUSEMENU))) {
            Log::info("PAUSE PRESS PROCESSING - ipad = %d, NavigateToScene\n",
                      player->GetXboxPad());
            ui.PlayUISFX(eSFX_Press);
#if !defined(ENABLE_JAVA_GUIS)
            ui.NavigateToScene(iPad, eUIScene_PauseMenu, nullptr,
                               eUILayer_Scene);
#endif
        }

        if ((player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_DROP)) &&
            gameMode->isInputAllowed(MINECRAFT_ACTION_DROP)) {
            player->drop();
        }

        uint64_t ullButtonsPressed = player->ullButtonsPressed;

        bool selected = false;
        {
            int hotbarSlot = PlatformInput.GetHotbarSlotPressed(iPad);
            if (hotbarSlot >= 0 && hotbarSlot <= 9) {
                player->inventory->selected = hotbarSlot;
                selected = true;
            }
        }
        if (selected || wheel != 0 ||
            (player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_DROP))) {
            std::string itemName = "";
            std::shared_ptr<ItemInstance> selectedItem =
                player->getSelectedItem();
            // Dropping items happens over network, so if we only have one then
            // assume that we dropped it and should hide the item
            int iCount = 0;

            if (selectedItem != nullptr) iCount = selectedItem->GetCount();
            if (selectedItem != nullptr && !((player->ullButtonsPressed &
                                              (1LL << MINECRAFT_ACTION_DROP)) &&
                                             selectedItem->GetCount() == 1)) {
                itemName = selectedItem->getHoverName();
            }
            if (!(player->ullButtonsPressed & (1LL << MINECRAFT_ACTION_DROP)) ||
                (selectedItem != nullptr && selectedItem->GetCount() <= 1))
                ui.SetSelectedItem(iPad, itemName);
        }
    } else {
        // 4J-PB
        // if (PlatformInput.GetValue(iPad, ACTION_MENU_CANCEL) > 0 &&
        //     gameMode->isInputAllowed(ACTION_MENU_CANCEL)) {
        //     setScreen(nullptr);
        // }
    }

    // monitor for keyboard input
    // #ifndef _CONTENT_PACKAGE
    // 	if(!(ui.GetMenuDisplayed(iPad)))
    // 	{
    // 		char wchInput;
    // 		if(PlatformInput.InputDetected(iPad,&wchInput))
    // 		{
    // 			printf("Input Detected!\n");
    //
    // 			// see if we can react to this
    // 			if(gameServices().getXuiAction(iPad)==eAppAction_Idle)
    // 			{
    // 				gameServices().setAction(iPad,eAppAction_DebugText,(void*)wchInput);
    // 			}
    // 		}
    // 	}
    // #endif

    if (level != nullptr) {
        if (player != nullptr) {
            recheckPlayerIn++;
            if (recheckPlayerIn == 30) {
                recheckPlayerIn = 0;
                level->ensureAdded(player);
            }
        }
        // 4J Changed - We are setting the difficulty the same as the server so
        // that leaderboard updates work correctly
        // level->difficulty = options->difficulty;
        // if (level->isClientSide) level->difficulty = Difficulty::HARD;
        if (!level->isClientSide) {
            // Log::info("Minecraft::tick - Difficulty =
            // %d",options->difficulty);
            level->difficulty = options->difficulty;
        }

        if (!pause) gameRenderer->tick(bFirst);

        // 4J - we want to tick each level once only per frame, and do it when a
        // player that is actually in that level happens to be active. This is
        // important as things that get called in the level tick (eg the
        // levellistener) eventually end up working out what the current level
        // is by determing it from the current player. Use flags here to make
        // sure each level is only ticked the once.
        static unsigned int levelsTickedFlags;
        if (bFirst) {
            levelsTickedFlags = 0;

#if !defined(DISABLE_LEVELTICK_THREAD)
            levelTickEventQueue->waitForFinish();

#endif
            SparseLightStorage::tick();     // 4J added
            CompressedTileStorage::tick();  // 4J added
            SparseDataStorage::tick();      // 4J added
        }

        for (unsigned int i = 0; i < levels.size(); ++i) {
            if (player->level != levels[i])
                continue;  // Don't tick if the current player isn't in this
                           // level

            // 4J - this doesn't fully tick the animateTick here, but does
            // register this player's position. The actual work is now done in
            // Level::animateTickDoWork() so we can take into account multiple
            // players in the one level.
            if (!pause && levels[i] != nullptr)
                levels[i]->animateTick(std::floor(player->x),
                                       std::floor(player->y),
                                       std::floor(player->z));

            if (levelsTickedFlags & (1 << i))
                continue;  // Don't tick further if we've already ticked this
                           // level this frame
            levelsTickedFlags |= (1 << i);

            if (!pause) levelRenderer->tick();

            // if (!pause && player!=null) {
            // if (player != null && !level.entities.contains(player)) {
            // level.addEntity(player);
            // }
            // }
            if (levels[i] != nullptr) {
                if (!pause) {
                    if (levels[i]->skyFlashTime > 0) levels[i]->skyFlashTime--;
                    levels[i]->tickEntities();
                }

                // optimisation to set the culling off early, in parallel with
                // other stuff

                // 4J Stu - We are always online, but still could be paused
                if (!pause)  // || isClientSide())
                {
                    // Log::info("Minecraft::tick spawn settings -
                    // Difficulty = %d",options->difficulty);
                    levels[i]->setSpawnSettings(level->difficulty > 0, true);
#if defined(DISABLE_LEVELTICK_THREAD)
                    levels[i]->tick();
#else
                    levelTickEventQueue->sendEvent(levels[i]);
#endif
                }
            }
        }

        if (bFirst) {
            if (!pause) particleEngine->tick();
        }

        // 4J Stu - Keep ticking the connections if paused so that they don't
        // time out
        if (pause) tickAllConnections();
        // player->tick();
    }

    // if (Keyboard.isKeyDown(Keyboard.KEY_NUMPAD7) ||
    // Keyboard.isKeyDown(Keyboard.KEY_Q)) rota++;
    // if (Keyboard.isKeyDown(Keyboard.KEY_NUMPAD9) ||
    // Keyboard.isKeyDown(Keyboard.KEY_E)) rota--;
    // 4J removed
    // lastTickTime = System::currentTimeMillis();
}

void Minecraft::reloadSound() {
    //    System.out.println("FORCING RELOAD!");		// 4J - removed
    soundEngine = new SoundEngine();
    soundEngine->init(options);
    bgLoader->forceReload();
}

bool Minecraft::isClientSide() {
    return level != nullptr && level->isClientSide;
}

void Minecraft::selectLevel(ConsoleSaveFile* saveFile,
                            const std::string& levelId,
                            const std::string& levelName,
                            LevelSettings* levelSettings) {}

bool Minecraft::saveSlot(int slot, const std::string& name) { return false; }

bool Minecraft::loadSlot(const std::string& userName, int slot) {
    return false;
}

void Minecraft::releaseLevel(int message) {
    // this->level = nullptr;
    setLevel(nullptr, message);
}

// 4J Stu - This code was within setLevel, but I moved it out so that I can call
// it at a better time when exiting from an online game
void Minecraft::forceStatsSave(int idx) {
    // 4J Gordon: Force a stats save
    stats[idx]->save(idx, true);

    // 4J Gordon: If the player is signed in, save the leaderboards
    if (PlatformProfile.IsSignedInLive(idx)) {
        int tempLockedProfile = PlatformProfile.GetLockedProfile();
        PlatformProfile.SetLockedProfile(idx);
        stats[idx]->saveLeaderboards();
        PlatformProfile.SetLockedProfile(tempLockedProfile);
    }
}

// 4J Added
MultiPlayerLevel* Minecraft::getLevel(int dimension) {
    if (dimension == -1)
        return levels[1];
    else if (dimension == 1)
        return levels[2];
    else
        return levels[0];
}

// 4J Stu - Removed as redundant with default values in params.
// void Minecraft::setLevel(Level *level, bool doForceStatsSave /*= true*/)
//{
//	setLevel(level, -1, nullptr, doForceStatsSave);
//}

// Also causing ambiguous call for some reason
// as it is matching shared_ptr<Player> from the func below with bool from this
// one
// void Minecraft::setLevel(Level *level, const string& message, bool
// doForceStatsSave /*= true*/)
//{
//	setLevel(level, message, nullptr, doForceStatsSave);
//}

void Minecraft::forceaddLevel(MultiPlayerLevel* level) {
    int dimId = level->dimension->id;
    if (dimId == -1)
        levels[1] = level;
    else if (dimId == 1)
        levels[2] = level;
    else
        levels[0] = level;
}

void Minecraft::setLevel(MultiPlayerLevel* level, int message /*=-1*/,
                         std::shared_ptr<Player> forceInsertPlayer /*=nullptr*/,
                         bool doForceStatsSave /*=true*/,
                         bool bPrimaryPlayerSignedOut /*=false*/) {
    std::lock_guard<std::recursive_mutex> lock(m_setLevelCS);
    bool playerAdded = false;
    this->cameraTargetPlayer = nullptr;

    if (progressRenderer != nullptr) {
        this->progressRenderer->progressStart(message);
        this->progressRenderer->progressStage(-1);
    }

    // 4J-PB - since we now play music in the menu, just let it keep playing
    // soundEngine->playStreaming("", 0, 0, 0, 0, 0);

    // 4J - stop update thread from processing this level, which blocks until it
    // is safe to move on - will be re-enabled if we set the level to be
    // non-nullptr
    gameRenderer->DisableUpdateThread();

    for (unsigned int i = 0; i < levels.size(); ++i) {
        // 4J We only need to save out in multiplayer is we are setting the
        // level to nullptr If we ever go back to making single player only then
        // this will not work properly!
        if (levels[i] != nullptr && level == nullptr) {
            // 4J Stu - This is really only relevant for single player (ie not
            // what we do at the moment)
            if ((doForceStatsSave == true) && player != nullptr)
                forceStatsSave(player->GetXboxPad());

            // 4J Stu - Added these for the case when we exit a level so we are
            // setting the level to nullptr The level renderer needs to have
            // it's stored level set to nullptr so that it doesn't break next
            // time we set one
            if (levelRenderer != nullptr) {
                for (unsigned int p = 0; p < XUSER_MAX_COUNT; ++p) {
                    levelRenderer->setLevel(p, nullptr);
                }
            }
            if (particleEngine != nullptr) particleEngine->setLevel(nullptr);
        }
    }
    // 4J If we are setting the level to nullptr then we are exiting, so delete
    // the levels
    if (level == nullptr) {
        if (levels[0] != nullptr) {
            delete levels[0];
            levels[0] = nullptr;

            // Both level share the same savedDataStorage
            if (levels[1] != nullptr) levels[1]->savedDataStorage = nullptr;
        }
        if (levels[1] != nullptr) {
            delete levels[1];
            levels[1] = nullptr;
        }
        if (levels[2] != nullptr) {
            delete levels[2];
            levels[2] = nullptr;
        }

        // Delete all the player objects
        for (unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
            std::shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[idx];
            if (mplp != nullptr && mplp->connection != nullptr) {
                delete mplp->connection;
                mplp->connection = nullptr;
            }

            if (localgameModes[idx] != nullptr) {
                delete localgameModes[idx];
                localgameModes[idx] = nullptr;
            }

            if (m_pendingLocalConnections[idx] != nullptr) {
                delete m_pendingLocalConnections[idx];
                m_pendingLocalConnections[idx] = nullptr;
            }

            localplayers[idx] = nullptr;
        }
        // If we are removing the primary player then there can't be a valid
        // gamemode left anymore, this pointer will be referring to the one
        // we've just deleted
        gameMode = nullptr;
        // Remove references to player
        player = nullptr;
        cameraTargetPlayer = nullptr;
        EntityRenderDispatcher::instance->cameraEntity = nullptr;
        TileEntityRenderDispatcher::instance->cameraEntity = nullptr;
    }
    this->level = level;

    if (level != nullptr) {
        int dimId = level->dimension->id;
        if (dimId == -1)
            levels[1] = level;
        else if (dimId == 1)
            levels[2] = level;
        else
            levels[0] = level;

        // If no player has been set, then this is the first level to be set
        // this game, so set up a primary player & initialise some other things
        if (player == nullptr) {
            int iPrimaryPlayer = PlatformInput.GetPrimaryPad();

            player = gameMode->createPlayer(level);

            PlayerUID playerXUIDOffline = INVALID_XUID;
            PlayerUID playerXUIDOnline = INVALID_XUID;
            PlatformProfile.GetXUID(iPrimaryPlayer, &playerXUIDOffline, false);
            PlatformProfile.GetXUID(iPrimaryPlayer, &playerXUIDOnline, true);
            player->setXuid(playerXUIDOffline);
            player->setOnlineXuid(playerXUIDOnline);

            player->m_displayName =
                PlatformProfile.GetDisplayName(iPrimaryPlayer);

            player->resetPos();
            gameMode->initPlayer(player);

            player->SetXboxPad(iPrimaryPlayer);

            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                m_pendingLocalConnections[i] = nullptr;
                if (i != iPrimaryPlayer) localgameModes[i] = nullptr;
            }
        }

        if (player != nullptr) {
            player->resetPos();
            // gameMode.initPlayer(player);
            if (level != nullptr) {
                level->addEntity(player);
                playerAdded = true;
            }
        }

        if (player->input != nullptr) delete player->input;
        player->input = new Input();

        if (levelRenderer != nullptr)
            levelRenderer->setLevel(player->GetXboxPad(), level);
        if (particleEngine != nullptr) particleEngine->setLevel(level);

        gameMode->adjustPlayer(player);

        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            m_pendingLocalConnections[i] = nullptr;
        }
        updatePlayerViewportAssignments();

        this->cameraTargetPlayer = player;

        // 4J - allow update thread to start processing the level now both it &
        // the player should be ok
        gameRenderer->EnableUpdateThread();
    } else {
        levelSource->clearAll();
        player = nullptr;

        // Clear all players if the new level is nullptr
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            if (m_pendingLocalConnections[i] != nullptr)
                m_pendingLocalConnections[i]->close();
            m_pendingLocalConnections[i] = nullptr;
            localplayers[i] = nullptr;
            localgameModes[i] = nullptr;
        }
    }

    //    System.gc();	// 4J - removed
    // 4J removed
    // this->lastTickTime = 0;
}

void Minecraft::prepareLevel(int title) {
    if (progressRenderer != nullptr) {
        this->progressRenderer->progressStart(title);
        this->progressRenderer->progressStage(IDS_PROGRESS_BUILDING_TERRAIN);
    }
    int r = 128;
    if (gameMode->isCutScene()) r = 64;
    int pp = 0;
    int max = r * 2 / 16 + 1;
    max = max * max;
    ChunkSource* cs = level->getChunkSource();

    Pos* spawnPos = level->getSharedSpawnPos();
    if (player != nullptr) {
        spawnPos->x = (int)player->x;
        spawnPos->z = (int)player->z;
    }

    for (int x = -r; x <= r; x += 16) {
        for (int z = -r; z <= r; z += 16) {
            if (progressRenderer != nullptr)
                this->progressRenderer->progressStagePercentage((pp++) * 100 /
                                                                max);
            level->getTile(spawnPos->x + x, 64, spawnPos->z + z);
            // if (!gameMode->isCutScene()) {
            //     while (level->updateLights());
            // }
        }
    }
    delete spawnPos;
    if (!gameMode->isCutScene()) {
        if (progressRenderer != nullptr)
            this->progressRenderer->progressStage(
                IDS_PROGRESS_SIMULATING_WORLD);
        max = 2000;
    }
}

void Minecraft::fileDownloaded(const std::string& name, File* file) {
    int p = (int)name.find("/");
    std::string category = name.substr(0, p);
    std::string name2 = name.substr(p + 1);
    toLower(category);
    if (category == "sound") {
        soundEngine->add(name, file);
    } else if (category == "newsound") {
        soundEngine->add(name, file);
    } else if (category == "streaming") {
        soundEngine->addStreaming(name, file);
    } else if (category == "music") {
        soundEngine->addMusic(name, file);
    } else if (category == "newmusic") {
        soundEngine->addMusic(name, file);
    }
}

std::string Minecraft::gatherStats1() {
    // return levelRenderer->gatherStats1();
    return "Time to autosave: " +
           toWString<int64_t>(gameServices().secondsToAutosave()) + "s";
}

std::string Minecraft::gatherStats2() {
    return NetworkService.GatherStats();
    // return levelRenderer->gatherStats2();
}

std::string Minecraft::gatherStats3() {
    return NetworkService.GatherRTTStats();
    // return "P: " + particleEngine->countParticles() + ". T: " +
    // level->gatherStats();
}

std::string Minecraft::gatherStats4() {
    return level->gatherChunkSourceStats();
}

void Minecraft::respawnPlayer(int iPad, int dimension, int newEntityId) {
    gameRenderer
        ->DisableUpdateThread();  // 4J - don't do updating whilst we are
                                  // adjusting the player & localplayer array
    std::shared_ptr<MultiplayerLocalPlayer> localPlayer = localplayers[iPad];

    level->validateSpawn();
    level->removeAllPendingEntityRemovals();

    if (localPlayer != nullptr) {
        level->removeEntity(localPlayer);
    }

    std::shared_ptr<Player> oldPlayer = localPlayer;
    cameraTargetPlayer = nullptr;

    // 4J-PB - copy and set the players xbox pad
    int iTempPad = localPlayer->GetXboxPad();
    int iTempScreenSection = localPlayer->m_iScreenSection;
    EDefaultSkins skin = localPlayer->getPlayerDefaultSkin();
    player = localgameModes[iPad]->createPlayer(level);

    PlayerUID playerXUIDOffline = INVALID_XUID;
    PlayerUID playerXUIDOnline = INVALID_XUID;
    PlatformProfile.GetXUID(iTempPad, &playerXUIDOffline, false);
    PlatformProfile.GetXUID(iTempPad, &playerXUIDOnline, true);
    player->setXuid(playerXUIDOffline);
    player->setOnlineXuid(playerXUIDOnline);
    player->setIsGuest(PlatformProfile.IsGuest(iTempPad));

    player->m_displayName = PlatformProfile.GetDisplayName(iPad);

    player->SetXboxPad(iTempPad);

    player->m_iScreenSection = iTempScreenSection;
    player->setPlayerIndex(localPlayer->getPlayerIndex());
    player->setCustomSkin(localPlayer->getCustomSkin());
    player->setPlayerDefaultSkin(skin);
    player->setCustomCape(localPlayer->getCustomCape());
    player->m_sessionTimeStart = localPlayer->m_sessionTimeStart;
    player->m_dimensionTimeStart = localPlayer->m_dimensionTimeStart;
    player->setPlayerGamePrivilege(Player::ePlayerGamePrivilege_All,
                                   localPlayer->getAllPlayerGamePrivileges());

    player->SetThirdPersonView(oldPlayer->ThirdPersonView());

    // Fix for #63021 - TU7: Content: UI: Travelling from/to the Nether results
    // in switching currently held item to another. Fix for #81759 - TU9:
    // Content: Gameplay: Entering The End Exit Portal replaces the Player's
    // currently held item with the first one from the Quickbar
    if (localPlayer->getHealth() > 0 && localPlayer->y > -64) {
        player->inventory->selected = localPlayer->inventory->selected;
    }

    // Set the animation override if the skin has one
    std::uint32_t dwSkinID =
        gameServices().getSkinIdFromPath(player->customTextureUrl);
    if (GET_IS_DLC_SKIN_FROM_BITMASK(dwSkinID)) {
        player->setAnimOverrideBitmask(
            player->getSkinAnimOverrideBitmask(dwSkinID));
    }

    player->dimension = dimension;
    cameraTargetPlayer = player;

    // 4J-PB - are we the primary player or a local player?
    if (iPad == PlatformInput.GetPrimaryPad()) {
        createPrimaryLocalPlayer(iPad);

        // update the debugoptions
        gameServices().setGameSettingsDebugMask(
            PlatformInput.GetPrimaryPad(),
            gameServices().debugGetMask(-1, true));
    } else {
        storeExtraLocalPlayer(iPad);
    }

    player->setShowOnMaps(
        gameServices().getGameHostOption(eGameHostOption_Gamertags) != 0
            ? true
            : false);

    player->resetPos();
    level->addEntity(player);
    gameMode->initPlayer(player);

    if (player->input != nullptr) delete player->input;
    player->input = new Input();
    player->entityId = newEntityId;
    player->animateRespawn();
    gameMode->adjustPlayer(player);

    // 4J - added isClientSide check here
    if (!level->isClientSide) {
        prepareLevel(IDS_PROGRESS_RESPAWNING);
    }

    // 4J Added for multiplayer. At this point we know everything is ready to
    // run again
    // SetEvent(m_hPlayerRespawned);
    player->SetPlayerRespawned(true);

    if (dynamic_cast<DeathScreen*>(screen) != nullptr) setScreen(nullptr);

    gameRenderer->EnableUpdateThread();
}

void Minecraft::start(const std::string& name, const std::string& sid,
                      IPlatformLeaderboard& leaderboard) {
    startAndConnectTo(name, sid, "", leaderboard);
}

void Minecraft::startAndConnectTo(const std::string& name,
                                  const std::string& sid,
                                  const std::string& url,
                                  IPlatformLeaderboard& leaderboard) {
    bool fullScreen = false;
    std::string userName = name;

    /* 4J - removed window handling things here
    final Frame frame = new Frame("Minecraft");
    Canvas canvas = new Canvas();
    frame.setLayout(new BorderLayout());

    frame.add(canvas, BorderLayout.CENTER);

    // OverlayLayout oll = new OverlayLayout(frame);
    // oll.addLayoutComponent(canvas, BorderLayout.CENTER);
    // oll.addLayoutComponent(new JLabel("TEST"), BorderLayout.EAST);

    canvas.setPreferredSize(new Dimension(854, 480));
    frame.pack();
    frame.setLocationRelativeTo(null);
    */

    Minecraft* minecraft;
    // 4J - was new Minecraft(frame, canvas, nullptr, 854, 480, fullScreen);

    minecraft =
        new Minecraft(nullptr, nullptr, nullptr, 1280, 720, fullScreen,
                      leaderboard);

    /* - 4J - removed
    {
    @Override
    public void onCrash(CrashReport crashReport) {
    frame.removeAll();
    frame.add(new CrashInfoPanel(crashReport), BorderLayout.CENTER);
    frame.validate();
    }
    }; */

    /* 4J - removed
    final Thread thread = new Thread(minecraft, "Minecraft main thread");
    thread.setPriority(Thread.MAX_PRIORITY);
    */
    minecraft->serverDomain = "www.minecraft.net";

    {
        if (userName != "" &&
            sid != "")  // 4J - username & side were compared with nullptr
                        // rather than empty strings
        {
            minecraft->user = new User(userName, sid);
        } else {
            minecraft->user = new User(
                "Player" + toWString<int>(System::currentTimeMillis() % 1000),
                "");
        }
    }
    // else
    //{
    //	minecraft->user = new DemoUser();
    // }

    /* 4J - TODO
    if (url != nullptr)
    {
    String[] tokens = url.split(":");
    minecraft.connectTo(tokens[0], Integer.parseInt(tokens[1]));
    }
    */

    /* 4J - removed
    frame.setVisible(true);
    frame.addWindowListener(new WindowAdapter() {
    public void windowClosing(WindowEvent arg0) {
    minecraft.stop();
    try {
    thread.join();
    } catch (InterruptedException e) {
    e.printStackTrace();
    }
    System.exit(0);
    }
    });
    */
    // 4J - TODO - consider whether we need to actually create a thread here
    minecraft->run();
}

ClientConnection* Minecraft::getConnection(int iPad) {
    return localplayers[iPad]->connection;
}

// 4J-PB - so we can access this from within our xbox game loop
Minecraft* Minecraft::GetInstance() { return m_instance; }

bool useLomp = false;

int g_iMainThreadId;

void Minecraft::main(IPlatformLeaderboard& leaderboard) {
    std::string name;
    std::string sessionId;

    // g_iMainThreadId = GetCurrentThreadId();

    useLomp = true;

    Minecraft_RunStaticCtors();
    EntityRenderDispatcher::staticCtor();
    TileEntityRenderDispatcher::staticCtor();
    User::staticCtor();
    Tutorial::staticCtor();
    ColourTable::staticCtor();
    gameServices().loadDefaultGameRules();

#if defined(_LARGE_WORLDS)
    LevelRenderer::staticCtor();
#endif

    // 4J Stu - This block generates XML for the game rules schema

    // 4J-PB - Can't call this for the first 5 seconds of a game - MS rule
    {
        name =
            "Player" + toWString<int64_t>(System::currentTimeMillis() % 1000);
        sessionId = "-";
        /* 4J - TODO - get a session ID from somewhere?
        if (args.size() > 0) name = args[0];
        sessionId = "-";
        if (args.size() > 1) sessionId = args[1];
        */
    }

    // Common for all platforms
    IUIScene_CreativeMenu::staticCtor();

    // On PS4, we call Minecraft::Start from another thread, as this has been
    // timed taking ~2.5 seconds and we need to do some basic rendering stuff so
    // that we don't break the TRCs on SubmitDone calls
    Minecraft::start(name, sessionId, leaderboard);
}

bool Minecraft::renderNames() {
    if (m_instance == nullptr || !m_instance->options->hideGui) {
        return true;
    }
    return false;
}

bool Minecraft::useFancyGraphics() {
    return (m_instance != nullptr && m_instance->options->fancyGraphics);
}

bool Minecraft::useAmbientOcclusion() {
    return (m_instance != nullptr &&
            m_instance->options->ambientOcclusion != Options::AO_OFF);
}

bool Minecraft::renderDebug() {
    return (m_instance != nullptr && m_instance->options->renderDebug);
}

bool Minecraft::handleClientSideCommand(const std::string& chatMessage) {
    return false;
}

int Minecraft::maxSupportedTextureSize() {
    // 4J Force value
    return 1024;

    // for (int texSize = 16384; texSize > 0; texSize >>= 1) {
    //	GL11.glTexImage2D(GL11.GL_PROXY_TEXTURE_2D, 0, GL11.GL_RGBA, texSize,
    // texSize, 0, GL11.GL_RGBA, GL11.GL_UNSIGNED_BYTE, (ByteBuffer) null);
    // final int width = GL11.glGetTexLevelParameteri(GL11.GL_PROXY_TEXTURE_2D,
    // 0, GL11.GL_TEXTURE_WIDTH); 	if (width != 0) { 		return
    // texSize;
    //	}
    // }
    // return -1;
}

void Minecraft::delayTextureReload() { reloadTextures = true; }

int64_t Minecraft::currentTimeMillis() {
    return System::currentTimeMillis();  //(Sys.getTime() * 1000) /
                                         // Sys.getTimerResolution();
}

/*void Minecraft::handleMouseDown(int button, bool down)
{
if (gameMode->instaBuild) return;
if (!down) missTime = 0;
if (button == 0 && missTime > 0) return;

if (down && hitResult != nullptr && hitResult->type == HitResult::TILE && button
== 0)
{
int x = hitResult->x;
int y = hitResult->y;
int z = hitResult->z;
gameMode->continueDestroyBlock(x, y, z, hitResult->f);
particleEngine->crack(x, y, z, hitResult->f);
}
else
{
gameMode->stopDestroyBlock();
}
}

void Minecraft::handleMouseClick(int button)
{
if (button == 0 && missTime > 0) return;
if (button == 0)
{
Log::info("handleMouseClick - Player %d is
swinging\n",player->GetXboxPad()); player->swing();
}

bool mayUse = true;

//	* if (button == 1) { ItemInstance item =
//	* player.inventory.getSelected(); if (item != null) { if
//	* (gameMode.useItem(player, item)) {
//	* gameRenderer.itemInHandRenderer.itemUsed(); return; } } }

// 4J-PB - Adding a special case in here for sleeping in a bed in a multiplayer
game - we need to wake up, and we don't have the inbedchatscreen with a button

if(button==1 && (player->isSleeping() && level != nullptr &&
level->isClientSide))
{
shared_ptr<MultiplayerLocalPlayer> mplp =
std::dynamic_pointer_cast<MultiplayerLocalPlayer>( player );

if(mplp) mplp->StopSleeping();

// 4J - TODO
//if (minecraft.player instanceof MultiplayerLocalPlayer)
//{
//    ClientConnection connection = ((MultiplayerLocalPlayer)
minecraft.player).connection;
//    connection.send(new PlayerCommandPacket(minecraft.player,
PlayerCommandPacket.STOP_SLEEPING));
//}
}

if (hitResult == nullptr)
{
if (button == 0 && !(dynamic_cast<CreativeMode *>(gameMode) != nullptr))
missTime = 10;
}
else if (hitResult->type == HitResult::ENTITY)
{
if (button == 0)
{
gameMode->attack(player, hitResult->entity);
}
if (button == 1)
{
gameMode->interact(player, hitResult->entity);
}
}
else if (hitResult->type == HitResult::TILE)
{
int x = hitResult->x;
int y = hitResult->y;
int z = hitResult->z;
int face = hitResult->f;

//	* if (button != 0) { if (hitResult.f == 0) y--; if (hitResult.f ==
//	* 1) y++; if (hitResult.f == 2) z--; if (hitResult.f == 3) z++; if
//	* (hitResult.f == 4) x--; if (hitResult.f == 5) x++; }

// if (isClientSide())
// {
// return;
// }

if (button == 0)
{
gameMode->startDestroyBlock(x, y, z, hitResult->f);
}
else
{
shared_ptr<ItemInstance> item = player->inventory->getSelected();
int oldCount = item != nullptr ? item->count : 0;
if (gameMode->useItemOn(player, level, item, x, y, z, face))
{
mayUse = false;
Log::info("Player %d is swinging\n",player->GetXboxPad());
player->swing();
}
if (item == nullptr)
{
return;
}

if (item->count == 0)
{
player->inventory->items[player->inventory->selected] = nullptr;
}
else if (item->count != oldCount)
{
gameRenderer->itemInHandRenderer->itemPlaced();
}
}
}

if (mayUse && button == 1)
{
shared_ptr<ItemInstance> item = player->inventory->getSelected();
if (item != nullptr)
{
if (gameMode->useItem(player, level, item))
{
gameRenderer->itemInHandRenderer->itemUsed();
}
}
}
}
*/

// 4J-PB
Screen* Minecraft::getScreen() { return screen; }

bool Minecraft::isTutorial() {
    return m_inFullTutorialBits > 0;

    /*if( gameMode != nullptr && gameMode->isTutorial() )
    {
    return true;
    }
    else
    {
    return false;
    }*/
}

void Minecraft::playerStartedTutorial(int iPad) {
    // If the app doesn't think we are in a tutorial mode then just ignore this
    // add
    if (gameServices().getTutorialMode())
        m_inFullTutorialBits = m_inFullTutorialBits | (1 << iPad);
}

void Minecraft::playerLeftTutorial(int iPad) {
    // 4J Stu - Fix for bug that was flooding Sentient with LevelStart events
    // If the tutorial bits are already 0 then don't need to update anything
    if (m_inFullTutorialBits == 0) {
        gameServices().setTutorialMode(false);
        return;
    }

    m_inFullTutorialBits = m_inFullTutorialBits & ~(1 << iPad);
    if (m_inFullTutorialBits == 0) {
        gameServices().setTutorialMode(false);
    }
}

int Minecraft::InGame_SignInReturned(void* pParam, bool bContinue, int iPad) {
    Minecraft* pMinecraftClass = (Minecraft*)pParam;

    if (NetworkService.IsInSession()) {
        // 4J Stu - There seems to be a bug in the signin ui call that enables
        // guest sign in. We never allow this within game, so make sure that
        // it's disabled Fix for #66516 - TCR #124: MPS Guest Support ; #001:
        // BAS Game Stability: TU8: The game crashes when second Guest signs-in
        // on console which takes part in Xbox LIVE multiplayer session.
        Log::info("Disabling Guest Signin\n");
        XEnableGuestSignin(false);
    }

    // If sign in succeded, we're in game and this player isn't already playing,
    // continue
    if (bContinue == true && NetworkService.IsInSession() &&
        pMinecraftClass->localplayers[iPad] == nullptr) {
        // It's possible that the player has not signed in - they can back out
        // or choose no for the converttoguest
        if (PlatformProfile.IsSignedIn(iPad)) {
            if (!NetworkService.SessionHasSpace()) {
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_OK;
                ui.RequestErrorMessage(IDS_MULTIPLAYER_FULL_TITLE,
                                       IDS_MULTIPLAYER_FULL_TEXT, uiIDA, 1);
            }
            // if this is a local game then profiles just need to be signed in
            else if (NetworkService.IsLocalGame() ||
                     (PlatformProfile.IsSignedInLive(iPad) &&
                      PlatformProfile.AllowedToPlayMultiplayer(iPad))) {
                if (pMinecraftClass->level->isClientSide) {
                    pMinecraftClass->addLocalPlayer(iPad);
                } else {
                    // create the local player for the iPad
                    std::shared_ptr<Player> player =
                        pMinecraftClass->localplayers[iPad];
                    if (player == nullptr) {
                        player = pMinecraftClass->createExtraLocalPlayer(
                            iPad, PlatformProfile.GetGamertag(iPad), iPad,
                            pMinecraftClass->level->dimension->id);
                    }
                }
            } else if (PlatformProfile.IsSignedInLive(
                           PlatformInput.GetPrimaryPad()) &&
                       !PlatformProfile.AllowedToPlayMultiplayer(iPad)) {
                // 4J Stu - Don't allow converting to guests as we don't allow
                // any guest sign-in while in the game Fix for #66516 - TCR
                // #124: MPS Guest Support ; #001: BAS Game Stability: TU8: The
                // game crashes when second Guest signs-in on console which
                // takes part in Xbox LIVE multiplayer session.
                // PlatformProfile.RequestConvertOfflineToGuestUI(
                // &Minecraft::InGame_SignInReturned, pMinecraftClass,iPad);
                unsigned int uiIDA[1];
                uiIDA[0] = IDS_CONFIRM_OK;
                ui.RequestErrorMessage(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE,
                                       IDS_NO_MULTIPLAYER_PRIVILEGE_JOIN_TEXT,
                                       uiIDA, 1, iPad);
            }
        }
    }
    return 0;
}

void Minecraft::tickAllConnections() {
    int oldIdx = getLocalPlayerIdx();
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; i++) {
        std::shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[i];
        if (mplp && mplp->connection) {
            setLocalPlayerIdx(i);
            mplp->connection->tick();
        }
    }
    setLocalPlayerIdx(oldIdx);
}

bool Minecraft::addPendingClientTextureRequest(const std::string& textureName) {
    auto it = find(m_pendingTextureRequests.begin(),
                   m_pendingTextureRequests.end(), textureName);
    if (it == m_pendingTextureRequests.end()) {
        m_pendingTextureRequests.push_back(textureName);
        return true;
    }
    return false;
}

void Minecraft::handleClientTextureReceived(const std::string& textureName) {
    auto it = find(m_pendingTextureRequests.begin(),
                   m_pendingTextureRequests.end(), textureName);
    if (it != m_pendingTextureRequests.end()) {
        m_pendingTextureRequests.erase(it);
    }
}

unsigned int Minecraft::getCurrentTexturePackId() {
    return skins->getSelected()->getId();
}

ColourTable* Minecraft::getColourTable() {
    TexturePack* selected = skins->getSelected();

    ColourTable* colours = selected->getColourTable();

    if (colours == nullptr) {
        colours = skins->getDefault()->getColourTable();
    }

    return colours;
}
