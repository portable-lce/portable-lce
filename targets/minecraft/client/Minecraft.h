#pragma once
#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "java/File.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/network/packet/DisconnectPacket.h"
#include "platform/thread/C4JThread.h"
#include "platform/PlatformTypes.h"
#include "platform/stubs.h"
#include "platform/leaderboard/leaderboard.h"

class Timer;
class MultiPlayerLevel;
class LevelRenderer;
class MultiplayerLocalPlayer;
class Player;
class Mob;
class ParticleEngine;
class User;
class Canvas;
class Textures;
class Font;
class Screen;
class ProgressRenderer;
class GameRenderer;
class BackgroundDownloader;
class HumanoidModel;
class HitResult;
class Options;
class ConsoleSoundEngine;
class MinecraftApplet;
class MouseHandler;
class TexturePackRepository;
class File;
class LevelStorageSource;
class StatsCounter;
class Component;
class Entity;
class AchievementPopup;
class WaterTexture;
class LavaTexture;
class Gui;
class ClientConnection;
class ConsoleSaveFile;
class ItemInHandRenderer;
class LevelSettings;
class ColourTable;
class MultiPlayerGameMode;
class LivingEntity;
class Level;
class ResourceLocation;

#if defined(linux)
#undef linux
#endif

class Minecraft {
private:
    enum OS { linux, solaris, windows, macos, unknown, xbox };

    static ResourceLocation DEFAULT_FONT_LOCATION;
    static ResourceLocation ALT_FONT_LOCATION;

public:
    static const std::string VERSION_STRING;
    Minecraft(Component* mouseComponent, Canvas* parent,
              MinecraftApplet* minecraftApplet, int width, int height,
              bool fullscreen, IPlatformLeaderboard& leaderboard);
    void init();

    // 4J - removed
    //    void crash(CrashReport crash);
    //    public abstract void onCrash(CrashReport crash);

private:
    static Minecraft* m_instance;

public:
    MultiPlayerGameMode* gameMode;

private:
    bool fullscreen;
    bool hasCrashed;

    C4JThread::EventQueue* levelTickEventQueue;

    static void levelTickUpdateFunc(void* pParam);
    static void levelTickThreadInitFunc();

public:
    int width, height;
    int width_phys, height_phys;  // 4J - added
    //    private OpenGLCapabilities openGLCapabilities;

private:
    Timer* timer;
    bool reloadTextures;

public:
    Level* oldLevel;  // 4J Stu added to keep a handle on an old level so we can
                      // delete it
    // void* m_hPlayerRespawned; // 4J Added so we can wait in menus until it
    // is done (for async in multiplayer)
public:
    MultiPlayerLevel* level;
    LevelRenderer* levelRenderer;
    std::shared_ptr<MultiplayerLocalPlayer> player;

    std::vector<MultiPlayerLevel*> levels;

    std::shared_ptr<MultiplayerLocalPlayer> localplayers[XUSER_MAX_COUNT];
    MultiPlayerGameMode* localgameModes[XUSER_MAX_COUNT];
    int localPlayerIdx;
    ItemInHandRenderer* localitemInHandRenderers[XUSER_MAX_COUNT];
    // 4J-PB - so we can have debugoptions in the server
    unsigned int uiDebugOptionsA[XUSER_MAX_COUNT];

    // 4J Stu - Added these so that we can show a Xui scene while connecting
    bool m_connectionFailed[XUSER_MAX_COUNT];
    DisconnectPacket::eDisconnectReason
        m_connectionFailedReason[XUSER_MAX_COUNT];
    ClientConnection* m_pendingLocalConnections[XUSER_MAX_COUNT];

    bool addLocalPlayer(
        int idx);  // Re-arrange the screen and start the connection
    void addPendingLocalConnection(int idx, ClientConnection* connection);
    void connectionDisconnected(int idx,
                                DisconnectPacket::eDisconnectReason reason) {
        m_connectionFailed[idx] = true;
        m_connectionFailedReason[idx] = reason;
    }

    std::shared_ptr<MultiplayerLocalPlayer> createExtraLocalPlayer(
        int idx, const std::string& name, int pad, int iDimension,
        ClientConnection* clientConnection = nullptr,
        MultiPlayerLevel* levelpassedin = nullptr);
    void createPrimaryLocalPlayer(int iPad);
    bool setLocalPlayerIdx(int idx);
    int getLocalPlayerIdx();
    void removeLocalPlayerIdx(int idx);
    void storeExtraLocalPlayer(int idx);
    void updatePlayerViewportAssignments();
    int unoccupiedQuadrant;  // 4J - added

    std::shared_ptr<LivingEntity> cameraTargetPlayer;
    std::shared_ptr<LivingEntity> crosshairPickMob;
    ParticleEngine* particleEngine;
    User* user;
    std::string serverDomain;
    Canvas* parent;
    bool appletMode;

    // 4J - per player ?
    volatile bool pause;
    volatile bool exitingWorldRightNow;

    Textures* textures;
    Font *font, *altFont;
    Screen* screen;
    ProgressRenderer* progressRenderer;
    GameRenderer* gameRenderer;

private:
    BackgroundDownloader* bgLoader;

    int ticks;
    // 4J-PB - moved to per player

    // int missTime;

    int orgWidth, orgHeight;

public:
    AchievementPopup* achievementPopup;

public:
    Gui* gui;
    // 4J - move to the per player structure?
    bool noRender;

    HumanoidModel* humanoidModel;
    HitResult* hitResult;
    Options* options;

protected:
    MinecraftApplet* minecraftApplet;

public:
    ConsoleSoundEngine* soundEngine;
    MouseHandler* mouseHandler;

public:
    TexturePackRepository* skins;
    File workingDirectory;

private:
    LevelStorageSource* levelSource;

public:
    static const int frameTimes_length = 512;
    static int64_t frameTimes[frameTimes_length];
    static const int tickTimes_length = 512;
    static int64_t tickTimes[tickTimes_length];
    static int frameTimePos;
    static int64_t warezTime;

private:
    int rightClickDelay;

public:
    // 4J- this should really be in localplayer
    StatsCounter* stats[4];

    // Borrowed from the composition root - Minecraft does not own the
    // backend. The reference is valid for the lifetime of the Minecraft
    // singleton (the leaderboard outlives Minecraft).
    IPlatformLeaderboard& leaderboard;

private:
    std::string connectToIp;
    int connectToPort;

public:
    void clearConnectionFailed();
    void connectTo(const std::string& server, int port);

private:
    void renderLoadingScreen();

public:
    void blit(int x, int y, int sx, int sy, int w, int h);

private:
    static File workDir;

public:
    static File getWorkingDirectory();
    static File getWorkingDirectory(const std::string& applicationName);

public:
    LevelStorageSource* getLevelSource();
    void setScreen(Screen* screen);

private:
    void checkGlError(const std::string& string);

public:
    void destroy();
    volatile bool running;
    std::string fpsString;
    void run();
    // 4J-PB - split the run into 3 parts so we can run it from our xbox game
    // loop
    static Minecraft* GetInstance();
    void run_middle();
    void run_end();

    void emergencySave();

    // 4J - removed
    // bool wasDown ;
private:
    //	void checkScreenshot();		// 4J - removed
    //    String grabHugeScreenshot(File workDir2, int width, int height, int
    //    ssWidth, int ssHeight);	// 4J - removed

    // 4J - per player thing?
    int64_t lastTimer;

    void renderFpsMeter(int64_t tickTime);

public:
    void stop();
    // 4J removed
    //    bool mouseGrabbed;
    //    void grabMouse();
    //    void releaseMouse();
    // 4J-PB - moved these into localplayer
    // void handleMouseDown(int button, bool down);
    // void handleMouseClick(int button);

    void pauseGame();
    //    void toggleFullScreen();	// 4J - removed
    bool pollResize();

private:
    void resize(int width, int height);

public:
    // 4J - Moved to per player
    // bool isRaining ;

    // 4J - Moved to per player
    // int64_t lastTickTime;

private:
    // 4J- per player?
    int recheckPlayerIn;
    void verify();

public:
    // 4J - added bFirst parameter, which is true for the first active viewport
    // in splitscreen 4J - added bUpdateTextures, which is true if the actual
    // renderer textures are to be updated - this will be true for the last time
    // this tick runs with bFirst true
    void tick(bool bFirst, bool bUpdateTextures);

private:
    void reloadSound();

public:
    bool isClientSide();
    void selectLevel(ConsoleSaveFile* saveFile, const std::string& levelId,
                     const std::string& levelName,
                     LevelSettings* levelSettings);
    // void toggleDimension(int targetDimension);
    bool saveSlot(int slot, const std::string& name);
    bool loadSlot(const std::string& userName, int slot);
    void releaseLevel(int message);
    // 4J Stu - Added the doForceStatsSave param
    // void setLevel(Level *level, bool doForceStatsSave = true);
    // void setLevel(Level *level, const std::string& message, bool
    // doForceStatsSave = true);
    void setLevel(MultiPlayerLevel* level, int message = -1,
                  std::shared_ptr<Player> forceInsertPlayer = nullptr,
                  bool doForceStatsSave = true,
                  bool bPrimaryPlayerSignedOut = false);
    // 4J-PB - added to force in the 'other' level when the main player creates
    // the level at game load time
    void forceaddLevel(MultiPlayerLevel* level);
    void prepareLevel(int title);  // 4J - changed to public
    void fileDownloaded(const std::string& name, File* file);
    //  OpenGLCapabilities getOpenGLCapabilities();	// 4J - removed

    std::string gatherStats1();
    std::string gatherStats2();
    std::string gatherStats3();
    std::string gatherStats4();

    void respawnPlayer(int iPad, int dimension, int newEntityId);
    static void start(const std::string& name, const std::string& sid,
                      IPlatformLeaderboard& leaderboard);
    static void startAndConnectTo(const std::string& name,
                                  const std::string& sid,
                                  const std::string& url,
                                  IPlatformLeaderboard& leaderboard);
    ClientConnection* getConnection(int iPad);  // 4J Stu added iPad param
    static void main(IPlatformLeaderboard& leaderboard);
    static bool renderNames();
    static bool useFancyGraphics();
    static bool useAmbientOcclusion();
    static bool renderDebug();
    bool handleClientSideCommand(const std::string& chatMessage);

    static int maxSupportedTextureSize();
    void delayTextureReload();
    static int64_t currentTimeMillis();

    static int InGame_SignInReturned(void* pParam, bool bContinue, int iPad);
    // 4J-PB
    Screen* getScreen();

    // 4J Stu
    void forceStatsSave(int idx);

    std::recursive_mutex m_setLevelCS;

private:
    // A bit field that store whether a particular quadrant is in the full
    // tutorial or not
    uint8_t m_inFullTutorialBits;

public:
    bool isTutorial();
    void playerStartedTutorial(int iPad);
    void playerLeftTutorial(int iPad);

    // 4J Added
    MultiPlayerLevel* getLevel(int dimension);

    void tickAllConnections();

    Level* animateTickLevel;  // 4J added

    // 4J - When a client requests a texture, it should add it to here while we
    // are waiting for it
    std::vector<std::string> m_pendingTextureRequests;
    std::vector<std::string>
        m_pendingGeometryRequests;  // additional skin box geometry

    // 4J Added
    bool addPendingClientTextureRequest(const std::string& textureName);
    void handleClientTextureReceived(const std::string& textureName);
    void clearPendingClientTextureRequests() {
        m_pendingTextureRequests.clear();
    }
    bool addPendingClientGeometryRequest(const std::string& textureName);
    void handleClientGeometryReceived(const std::string& textureName);
    void clearPendingClientGeometryRequests() {
        m_pendingGeometryRequests.clear();
    }

    unsigned int getCurrentTexturePackId();
    ColourTable* getColourTable();
};
