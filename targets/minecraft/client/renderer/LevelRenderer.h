#pragma once
#include "OffsettedRenderList.h"
#include "java/JavaIntHash.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelListener.h"
#include "minecraft/world/phys/AABB.h"
#include "platform/C4JThread.h"
#include "platform/network/NetTypes.h"

class ClipChunk;
class HitResult;
class Icon;
class ItemInstance;
class LivingEntity;
class Player;
class ResourceLocation;
#include <stddef.h>
#include <stdint.h>

#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class MultiPlayerLevel;
class Textures;
class Chunk;
class Minecraft;
class TileRenderer;
class Culler;
class Entity;
class TileEntity;
class Mob;
class Vec3;
class Particle;
class BlockDestructionProgress;
class IconRegister;
class Tesselator;

// AP - this is a system that works out which chunks actually need to be grouped
// together via the deferral system when doing chunk::rebuild. Doing this will
// reduce the number of chunks built in a single group and reduce the chance of
// seeing through the landscape when digging near the edges/corners of a chunk.
// I've added another chunk flag to mark a chunk critical so it swipes a bit
// from the reference count value (goes to 3 bits to 2). This works on Vita
// because it doesn't have split screen reference counting.

class LevelRenderer : public LevelListener {
    friend class Chunk;

private:
    static ResourceLocation MOON_LOCATION;
    static ResourceLocation MOON_PHASES_LOCATION;
    static ResourceLocation SUN_LOCATION;
    static ResourceLocation CLOUDS_LOCATION;
    static ResourceLocation END_SKY_LOCATION;

public:
    static const int CHUNK_XZSIZE = 16;
#if defined(_LARGE_WORLDS)
    static const int CHUNK_SIZE = 16;
#else
    static const int CHUNK_SIZE = 16;
#endif
    static const int CHUNK_Y_COUNT = Level::maxBuildHeight / CHUNK_SIZE;
#if defined(_WINDOWS64)
    static const int MAX_COMMANDBUFFER_ALLOCATIONS =
        512 * 1024 * 1024;  // 4J - added
#else
    static const int MAX_COMMANDBUFFER_ALLOCATIONS =
        55 * 1024 * 1024;  // 4J - added
#endif
public:
    LevelRenderer(Minecraft* mc, Textures* textures);

private:
    void renderStars();
    void createCloudMesh();  // 4J added
public:
    void setLevel(int playerIndex, MultiPlayerLevel* level);
    void allChanged();
    void allChanged(int playerIndex);

    // 4J-PB added
    void AddDLCSkinsToMemTextures();

public:
    void renderEntities(Vec3* cam, Culler* culler, float a);
    std::string gatherStats1();
    std::string gatherStats2();

private:
    void resortChunks(int xc, int yc, int zc);

public:
    int render(std::shared_ptr<LivingEntity> player, int layer, double alpha,
               bool updateChunks);

private:
    int renderChunks(int from, int to, int layer, double alpha);

public:
    int activePlayers();  // 4J - added
public:
    void renderSameAsLast(int layer, double alpha);
    void tick();
    void renderSky(float alpha);
    void renderHaloRing(float alpha);
    void renderClouds(float alpha);
    bool isInCloud(double x, double y, double z, float alpha);
    void renderAdvancedClouds(float alpha);
    bool updateDirtyChunks();

public:
    void renderHit(std::shared_ptr<Player> player, HitResult* h, int mode,
                   std::shared_ptr<ItemInstance> inventoryItem, float a);
    void renderDestroyAnimation(Tesselator* t, std::shared_ptr<Player> player,
                                float a);
    void renderHitOutline(std::shared_ptr<Player> player, HitResult* h,
                          int mode, float a);
    void render(AABB* b);
    void setDirty(int x0, int y0, int z0, int x1, int y1, int z1,
                  Level* level);  // 4J - added level param
    void tileChanged(int x, int y, int z);
    void tileLightChanged(int x, int y, int z);
    void setTilesDirty(int x0, int y0, int z0, int x1, int y1, int z1,
                       Level* level);  // 4J - added level param

    void cull(Culler* culler, float a);
    void playStreamingMusic(const std::string& name, int x, int y, int z);
    void playSound(int iSound, double x, double y, double z, float volume,
                   float pitch, float fSoundClipDist = 16.0f);
    void playSound(std::shared_ptr<Entity> entity, int iSound, double x,
                   double y, double z, float volume, float pitch,
                   float fSoundClipDist = 16.0f);
    void playSoundExceptPlayer(std::shared_ptr<Player> player, int iSound,
                               double x, double y, double z, float volume,
                               float pitch, float fSoundClipDist = 16.0f);
    void addParticle(ePARTICLE_TYPE eParticleType, double x, double y, double z,
                     double xa, double ya, double za);  // 4J added
    std::shared_ptr<Particle> addParticleInternal(ePARTICLE_TYPE eParticleType,
                                                  double x, double y, double z,
                                                  double xa, double ya,
                                                  double za);  // 4J added
    void entityAdded(std::shared_ptr<Entity> entity);
    void entityRemoved(std::shared_ptr<Entity> entity);
    void playerRemoved(std::shared_ptr<Entity> entity) {
    }  // 4J added - for when a player is removed from the level's player array,
       // not just the entity storage
    void skyColorChanged();
    void clear();
    void globalLevelEvent(int type, int sourceX, int sourceY, int sourceZ,
                          int data);
    void levelEvent(std::shared_ptr<Player> source, int type, int x, int y,
                    int z, int data);
    void destroyTileProgress(int id, int x, int y, int z, int progress);
    void registerTextures(IconRegister* iconRegister);

    struct RenderableTileEntityBucket {
        std::vector<std::shared_ptr<TileEntity> > tiles;
        std::unordered_map<TileEntity*, size_t> indexByTile;
    };

    typedef std::unordered_map<int, RenderableTileEntityBucket, IntKeyHash,
                               IntKeyEq>
        rteMap;

private:
    // debug
    int m_freezeticks;  // used to freeze the clouds

    // 4J - this block of declarations was scattered round the code but have
    // gathered everything into one place
    rteMap renderableTileEntities;  // 4J - changed - was
                                    // std::vector<std::shared_ptr<TileEntity>,
                                    // now hashed by chunk so we can find them
    typedef std::unordered_set<TileEntity*> rtePendingRemovalSet;
    typedef std::unordered_map<int, rtePendingRemovalSet, IntKeyHash, IntKeyEq>
        rtePendingRemovalMap;
    rtePendingRemovalMap m_renderableTileEntitiesPendingRemoval;
    std::mutex m_csRenderableTileEntities;
    MultiPlayerLevel* level[4];  // 4J - now one per player
    Textures* textures;
    //    std::vector<Chunk *> *sortedChunks[4];	// 4J - removed - not
    //    sorting our chunks anymore
    std::vector<ClipChunk> chunks[4];  // 4J - now one per player
    int lastPlayerCount[4];            // 4J - added
    int xChunks, yChunks, zChunks;
    int chunkLists;
    Minecraft* mc;
    TileRenderer* tileRenderer[4];  // 4J - now one per player
    int ticks;
    int starList, skyList, darkList, haloRingList;
    int cloudList;  // 4J added
    int xMinChunk, yMinChunk, zMinChunk;
    int xMaxChunk, yMaxChunk, zMaxChunk;
    int lastViewDistance;
    int noEntityRenderFrames;
    int totalEntities;
    int renderedEntities;
    int culledEntities;
    int chunkFixOffs;
    std::vector<Chunk*> _renderChunks;
    int frame;
    int repeatList;
    double xOld[4];  // 4J - now one per player
    double yOld[4];  // 4J - now one per player
    double zOld[4];  // 4J - now one per player

    int totalChunks, offscreenChunks, occludedChunks, renderedChunks,
        emptyChunks;
    static const int RENDERLISTS_LENGTH = 4;  // 4J - added
    OffsettedRenderList renderLists[RENDERLISTS_LENGTH];

#ifdef OCCLUSION_MODE_BFS
    void setGlobalChunkConnectivity(int index, uint64_t conn);
    uint64_t getGlobalChunkConnectivity(int index);
    std::vector<ClipChunk*> m_bfsGrid;
    std::vector<uint8_t> m_bfsVisitedFaces[4];
#endif

    std::unordered_map<int, BlockDestructionProgress*> destroyingBlocks;
    Icon** breakingTextures;

    void addRenderableTileEntity_Locked(
        int key, const std::shared_ptr<TileEntity>& tileEntity);
    void eraseRenderableTileEntity_Locked(RenderableTileEntityBucket& bucket,
                                          TileEntity* tileEntity);
    void queueRenderableTileEntityForRemoval_Locked(int key,
                                                    TileEntity* tileEntity);
    void retireRenderableTileEntitiesForChunkKey(int key);

public:
    void fullyFlagRenderableTileEntitiesToBeRemoved();  // 4J added

    std::recursive_mutex m_csDirtyChunks;
    bool m_nearDirtyChunk;

    // 4J - Destroyed Tile Management - these things added so we can track tiles
    // which have been recently destroyed, and provide temporary collision for
    // them until the render data has been updated to reflect this change
    class DestroyedTileManager {
    private:
        class RecentTile {
        public:
            int x;
            int y;
            int z;
            Level* level;
            std::vector<AABB> boxes;
            int timeout_ticks;
            bool rebuilt;
            RecentTile(int x, int y, int z, Level* level);
            ~RecentTile() = default;
        };
        std::mutex m_csDestroyedTiles;
        std::vector<RecentTile*> m_destroyedTiles;

    public:
        void destroyingTileAt(
            Level* level, int x, int y,
            int z);  // For game to let this manager know that a tile is about
                     // to be destroyed (must be called before it actually is)
        void updatedChunkAt(
            Level* level, int x, int y, int z,
            int veryNearCount);  // For chunk rebuilding to inform the manager
                                 // that a chunk (a 16x16x16 tile render chunk)
                                 // has been updated
        void addAABBs(
            Level* level, AABB* box,
            std::vector<AABB>* boxes);  // For game to get any AABBs that the
                                        // user should be colliding with as
                                        // render data has not yet been updated
        void tick();
        DestroyedTileManager();
        ~DestroyedTileManager();
    };
    DestroyedTileManager* destroyedTileManager;

    float destroyProgress;

    // 4J - added for new render list handling
    // This defines the maximum size of renderable level, must be big enough to
    // cope with actual size of level + view distance at each side so that we
    // can render the "infinite" sea at the edges
    static const int MAX_LEVEL_RENDER_SIZE[3];
    static const int DIMENSION_OFFSETS[3];
    // This is the TOTAL area of columns of chunks to be allocated for render
    // round the players. So for one player, it would be a region of
    // sqrt(PLAYER_RENDER_AREA) x sqrt(PLAYER_RENDER_AREA)
#if defined(_LARGE_WORLDS)
    static const int PLAYER_VIEW_DISTANCE =
        18;  // Straight line distance from centre to extent of visible world
    static const int PLAYER_RENDER_AREA =
        (PLAYER_VIEW_DISTANCE * PLAYER_VIEW_DISTANCE * 4);
#else
    static const int PLAYER_RENDER_AREA = 400;
#endif

    static int getDimensionIndexFromId(int id);
    static int getGlobalIndexForChunk(int x, int y, int z, Level* level);
    static int getGlobalIndexForChunk(int x, int y, int z, int dimensionId);
    static bool isGlobalIndexInSameDimension(int idx, Level* level);
    static int getGlobalChunkCount();
    static int getGlobalChunkCountForOverworld();

    // Get/set/clear individual flags
    bool getGlobalChunkFlag(int x, int y, int z, Level* level,
                            unsigned char flag, unsigned char shift = 0);
    void setGlobalChunkFlag(int x, int y, int z, Level* level,
                            unsigned char flag, unsigned char shift = 0);
    void setGlobalChunkFlag(int index, unsigned char flag,
                            unsigned char shift = 0);
    void clearGlobalChunkFlag(int x, int y, int z, Level* level,
                              unsigned char flag, unsigned char shift = 0);

#ifdef OCCLUSION_MODE_BFS
    static uint64_t* globalChunkConnectivity;
#endif

    // Get/set whole byte of flags
    unsigned char getGlobalChunkFlags(int x, int y, int z, Level* level);
    void setGlobalChunkFlags(int x, int y, int z, Level* level,
                             unsigned char flags);

    // Reference counting
    unsigned char incGlobalChunkRefCount(int x, int y, int z, Level* level);
    unsigned char decGlobalChunkRefCount(int x, int y, int z, Level* level);

    // Actual storage for flags
    unsigned char* globalChunkFlags;

    // The flag definitions
    static const int CHUNK_FLAG_COMPILED = 0x01;
    static const int CHUNK_FLAG_DIRTY = 0x02;
    static const int CHUNK_FLAG_EMPTY0 = 0x04;
    static const int CHUNK_FLAG_EMPTY1 = 0x08;
    static const int CHUNK_FLAG_EMPTYBOTH = 0x0c;
    static const int CHUNK_FLAG_NOTSKYLIT = 0x10;
#if defined(_CRITICAL_CHUNKS)
    static const int CHUNK_FLAG_CRITICAL = 0x20;
    static const int CHUNK_FLAG_CUT_OUT = 0x40;
    static const int CHUNK_FLAG_REF_MASK = 0x01;
    static const int CHUNK_FLAG_REF_SHIFT = 7;
#else
    static const int CHUNK_FLAG_REF_MASK = 0x07;
    static const int CHUNK_FLAG_REF_SHIFT = 5;
#endif

    XLockFreeStack<int> dirtyChunksLockFreeStack;

    bool dirtyChunkPresent;
    int64_t lastDirtyChunkFound;
    static const int FORCE_DIRTY_CHUNK_CHECK_PERIOD_MS = 250;

#if defined(_LARGE_WORLDS)
    static const int MAX_CONCURRENT_CHUNK_REBUILDS = 4;
    static const int MAX_CHUNK_REBUILD_THREADS =
        MAX_CONCURRENT_CHUNK_REBUILDS - 1;
    static Chunk permaChunk[MAX_CONCURRENT_CHUNK_REBUILDS];
    static C4JThread* rebuildThreads[MAX_CHUNK_REBUILD_THREADS];
    static C4JThread::EventArray* s_rebuildCompleteEvents;
    static C4JThread::Event* s_activationEventA[MAX_CHUNK_REBUILD_THREADS];
    static void staticCtor();
    static int rebuildChunkThreadProc(void* lpParam);

    std::mutex m_csChunkFlags;
#endif
    void nonStackDirtyChunksAdded();

    int checkAllPresentChunks(bool* faultFound);  // 4J - added for testing
};
