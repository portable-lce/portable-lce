#include "Chunk.h"
#include "platform/stubs.h"


#include <string.h>

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "platform/renderer/renderer.h"
#include "LevelRenderer.h"
#include "util/FrameProfiler.h"
#include "TileRenderer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/culling/Culler.h"
#include "minecraft/client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelSource.h"
#include "minecraft/world/level/Region.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "minecraft/world/phys/AABB.h"

int Chunk::updates = 0;

#if defined(_LARGE_WORLDS)
thread_local uint8_t* Chunk::m_tlsTileIds = nullptr;

void Chunk::CreateNewThreadStorage() {
    m_tlsTileIds = new unsigned char[16 * 16 * Level::maxBuildHeight];
}

void Chunk::ReleaseThreadStorage() { delete m_tlsTileIds; }

uint8_t* Chunk::GetTileIdsStorage() { return m_tlsTileIds; }
#else
// 4J Stu - Don't want this when multi-threaded
Tesselator* Chunk::t = Tesselator::getInstance();
#endif
LevelRenderer* Chunk::levelRenderer;

void Chunk::reconcileRenderableTileEntities(
    const std::vector<std::shared_ptr<TileEntity> >& renderableTileEntities) {
    int key =
        levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level);
    auto it = globalRenderableTileEntities->find(key);
    if (!renderableTileEntities.empty()) {
        std::unordered_set<TileEntity*> currentRenderableTileEntitySet;
        currentRenderableTileEntitySet.reserve(renderableTileEntities.size());
        for (size_t i = 0; i < renderableTileEntities.size(); i++) {
            currentRenderableTileEntitySet.insert(
                renderableTileEntities[i].get());
        }

        if (it != globalRenderableTileEntities->end()) {
            LevelRenderer::RenderableTileEntityBucket& existingBucket =
                it->second;

            for (auto it2 = existingBucket.tiles.begin();
                 it2 != existingBucket.tiles.end(); it2++) {
                TileEntity* tileEntity = (*it2).get();
                if (currentRenderableTileEntitySet.find(tileEntity) ==
                    currentRenderableTileEntitySet.end()) {
                    (*it2)->setRenderRemoveStage(
                        TileEntity::e_RenderRemoveStageFlaggedAtChunk);
                    levelRenderer->queueRenderableTileEntityForRemoval_Locked(
                        key, tileEntity);
                } else {
                    (*it2)->setRenderRemoveStage(
                        TileEntity::e_RenderRemoveStageKeep);
                }
            }

            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                renderableTileEntities[i]->setRenderRemoveStage(
                    TileEntity::e_RenderRemoveStageKeep);
                if (existingBucket.indexByTile.find(
                        renderableTileEntities[i].get()) ==
                    existingBucket.indexByTile.end()) {
                    levelRenderer->addRenderableTileEntity_Locked(
                        key, renderableTileEntities[i]);
                }
            }
        } else {
            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                renderableTileEntities[i]->setRenderRemoveStage(
                    TileEntity::e_RenderRemoveStageKeep);
                levelRenderer->addRenderableTileEntity_Locked(
                    key, renderableTileEntities[i]);
            }
        }
    } else if (it != globalRenderableTileEntities->end()) {
        for (auto it2 = it->second.tiles.begin(); it2 != it->second.tiles.end();
             it2++) {
            (*it2)->setRenderRemoveStage(
                TileEntity::e_RenderRemoveStageFlaggedAtChunk);
            levelRenderer->queueRenderableTileEntityForRemoval_Locked(
                key, (*it2).get());
        }
    }
}

// TODO - 4J see how input entity vector is set up and decide what way is best
// to pass this to the function
Chunk::Chunk(Level* level, LevelRenderer::rteMap& globalRenderableTileEntities,
             std::mutex& globalRenderableTileEntities_cs, int x, int y, int z,
             ClipChunk* clipChunk)
    : globalRenderableTileEntities(&globalRenderableTileEntities),
      globalRenderableTileEntities_cs(&globalRenderableTileEntities_cs) {
    clipChunk->visible = false;
    const double g = 6;
    bb = AABB(-g, -g, -g, XZSIZE + g, SIZE + g, XZSIZE + g);
    id = 0;

    this->level = level;
    // this->globalRenderableTileEntities = globalRenderableTileEntities;

    assigned = false;
    this->clipChunk = clipChunk;
    setPos(x, y, z);
}

void Chunk::setPos(int x, int y, int z) {
    if (assigned && (x == this->x && y == this->y && z == this->z)) return;

    reset();

    this->x = x;
    this->y = y;
    this->z = z;
    xm = x + XZSIZE / 2;
    ym = y + SIZE / 2;
    zm = z + XZSIZE / 2;
    clipChunk->xm = xm;
    clipChunk->ym = ym;
    clipChunk->zm = zm;

    clipChunk->globalIdx =
        LevelRenderer::getGlobalIndexForChunk(x, y, z, level);
#ifdef OCCLUSION_MODE_BFS
    levelRenderer->setGlobalChunkConnectivity(clipChunk->globalIdx, ~0ULL);
#endif

    // 4J - we're not using offsetted renderlists anymore, so just set the full
    // position of this chunk into x/y/zRenderOffs where it will be used
    // directly in the renderlist of this chunk
    xRenderOffs = x;
    yRenderOffs = y;
    zRenderOffs = z;
    xRender = 0;
    yRender = 0;
    zRender = 0;

    float g = 6.0f;

    clipChunk->aabb[0] = bb.x0 + x;
    clipChunk->aabb[1] = bb.y0 + y;
    clipChunk->aabb[2] = bb.z0 + z;
    clipChunk->aabb[3] = bb.x1 + x;
    clipChunk->aabb[4] = bb.y1 + y;
    clipChunk->aabb[5] = bb.z1 + z;

    assigned = true;

    {
        std::lock_guard<std::recursive_mutex> lock(
            levelRenderer->m_csDirtyChunks);
        unsigned char refCount =
            levelRenderer->incGlobalChunkRefCount(x, y, z, level);
        //	printf("\t\t [inc] refcount %d at %d, %d, %d\n",refCount,x,y,z);

        //	int idx = levelRenderer->getGlobalIndexForChunk(x, y, z, level);

        // If we're the first thing to be referencing this, mark it up as dirty
        // to get rebuilt
        if (refCount == 1) {
            //		printf("Setting %d %d %d dirty [%d]\n",x,y,z, idx);
            // Chunks being made dirty in this way can be very numerous (eg the
            // full visible area of the world at start up, or a whole edge of
            // the world when moving). On account of this, don't want to stick
            // them into our lock free queue that we would normally use for
            // letting the render update thread know about this chunk. Instead,
            // just set the flag to say this is dirty, and then pass a special
            // value of 1 through to the lock free stack which lets that thread
            // know that at least one chunk other than the ones in the stack
            // itself have been made dirty.
            levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                              LevelRenderer::CHUNK_FLAG_DIRTY);
        }
    }
}

void Chunk::translateToPos() {
    glTranslatef((float)xRenderOffs, (float)yRenderOffs, (float)zRenderOffs);
}

Chunk::Chunk() {}

void Chunk::makeCopyForRebuild(Chunk* source) {
    this->level = source->level;
    this->x = source->x;
    this->y = source->y;
    this->z = source->z;
    this->xRender = source->xRender;
    this->yRender = source->yRender;
    this->zRender = source->zRender;
    this->xRenderOffs = source->xRenderOffs;
    this->yRenderOffs = source->yRenderOffs;
    this->zRenderOffs = source->zRenderOffs;
    this->xm = source->xm;
    this->ym = source->ym;
    this->zm = source->zm;
    this->bb = source->bb;
    this->clipChunk = nullptr;
    this->id = source->id;
    this->globalRenderableTileEntities = source->globalRenderableTileEntities;
    this->globalRenderableTileEntities_cs =
        source->globalRenderableTileEntities_cs;
}

void Chunk::rebuild() {
    //	if (!dirty) return;

#if defined(_LARGE_WORLDS)
    Tesselator* t = Tesselator::getInstance();
#else
    Chunk::t = Tesselator::getInstance();  // 4J - added - static initialiser
                                           // being set at the wrong time
#endif

    updates++;

    int x0 = x;
    int y0 = y;
    int z0 = z;
    int x1 = x + XZSIZE;
    int y1 = y + SIZE;
    int z1 = z + XZSIZE;

    LevelChunk::touchedSky = false;

    //	unordered_set<shared_ptr<TileEntity> >
    // oldTileEntities(renderableTileEntities.begin(),renderableTileEntities.end());
    //// 4J removed this & next line 	renderableTileEntities.clear();

    std::vector<std::shared_ptr<TileEntity> >
        renderableTileEntities;  // 4J - added

    int r = 1;

    int lists = levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z,
                                                      level) *
                2;
    lists += levelRenderer->chunkLists;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 4J - optimisation begins.

    // Get the data for the level chunk that this render chunk is it (level
    // chunk is 16 x 16 x 128, render chunk is 16 x 16 x 16. We wouldn't have to
    // actually get all of it if the data was ordered differently, but currently
    // it is ordered by x then z then y so just getting a small range of y out
    // of it would involve getting the whole thing into the cache anyway.

#if defined(_LARGE_WORLDS)
    unsigned char* tileIds = GetTileIdsStorage();
#else
    static unsigned char tileIds[16 * 16 * Level::maxBuildHeight];
#endif
    std::vector<uint8_t> tileArray(16 * 16 * Level::maxBuildHeight);
    level->getChunkAt(x, z)->getBlockData(tileArray);
    memcpy(
        tileIds, tileArray.data(),
        16 * 16 * Level::maxBuildHeight);  // 4J - TODO - now our data has been
                                           // re-arranged, we could just extra
                                           // the vertical slice of this chunk
                                           // rather than the whole thing

    LevelSource* region =
        new Region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r, r);
    TileRenderer* tileRenderer =
        new TileRenderer(region, this->x, this->y, this->z, tileIds);

    // AP - added a caching system for Chunk::rebuild to take advantage of
    // Basically we're storing of copy of the tileIDs array inside the region so
    // that calls to Region::getTile can grab data more quickly from this array
    // rather than calling CompressedTileStorage. On the Vita the total thread
    // time spent in Region::getTile went from 20% to 4%.

    // We now go through the vertical section of this level chunk that we are
    // interested in and try and establish (1) if it is completely empty (2) if
    // any of the tiles can be quickly determined to not need rendering because
    // they are in the middle of other tiles and
    //     so can't be seen. A large amount (> 60% in tests) of tiles that call
    //     tesselateInWorld in the unoptimised version of this function fall
    //     into this category. By far the largest category of these are tiles in
    //     solid regions of rock.
    bool empty = true;
    {
        FRAME_PROFILE_SCOPE(ChunkPrepass);
        for (int yy = y0; yy < y1; yy++) {
            for (int zz = 0; zz < 16; zz++) {
                for (int xx = 0; xx < 16; xx++) {
                    // 4J Stu - tile data is ordered in 128 blocks of full
                    // width, lower 128 then upper 128
                    int indexY = yy;
                    int offset = 0;
                    if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }

                    unsigned char tileId =
                        tileIds[offset + (((xx + 0) << 11) | ((zz + 0) << 7) |
                                          (indexY + 0))];
                    if (tileId > 0) empty = false;

                    // Don't bother trying to work out neighbours for this tile
                    // if we are at the edge of the chunk - apart from the very
                    // bottom of the world where we shouldn't ever be able to
                    // see
                    if (yy == (Level::maxBuildHeight - 1)) continue;
                    if ((xx == 0) || (xx == 15)) continue;
                    if ((zz == 0) || (zz == 15)) continue;

                    // Establish whether this tile and its neighbours are all
                    // made of rock, dirt, unbreakable tiles, or have already
                    // been determined to meet this criteria themselves and have
                    // a tile of 255 set.
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx - 1) << 11) |
                                               ((zz + 0) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 1) << 11) |
                                               ((zz + 0) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 0) << 11) |
                                               ((zz - 1) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 0) << 11) |
                                               ((zz + 1) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    // Treat the bottom of the world differently - we shouldn't
                    // ever be able to look up at this, so consider tiles as
                    // invisible if they are surrounded on sides other than the
                    // bottom
                    if (yy > 0) {
                        int indexYMinusOne = yy - 1;
                        int yMinusOneOffset = 0;
                        if (indexYMinusOne >=
                            Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                            indexYMinusOne -=
                                Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                            yMinusOneOffset =
                                Level::COMPRESSED_CHUNK_SECTION_TILES;
                        }
                        tileId = tileIds[yMinusOneOffset + (((xx + 0) << 11) |
                                                            ((zz + 0) << 7) |
                                                            indexYMinusOne)];
                        if (!((tileId == Tile::stone_Id) ||
                              (tileId == Tile::dirt_Id) ||
                              (tileId == Tile::unbreakable_Id) ||
                              (tileId == 255)))
                            continue;
                    }
                    int indexYPlusOne = yy + 1;
                    int yPlusOneOffset = 0;
                    if (indexYPlusOne >=
                        Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexYPlusOne -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        yPlusOneOffset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }
                    tileId = tileIds[yPlusOneOffset + (((xx + 0) << 11) |
                                                       ((zz + 0) << 7) |
                                                       indexYPlusOne)];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;

                    // This tile is surrounded. Flag it as not requiring to be
                    // rendered by setting its id to 255.
                    tileIds[offset + (((xx + 0) << 11) | ((zz + 0) << 7) |
                                      (indexY + 0))] = 0xff;
                }
            }
        }
    }

    // Nothing at all to do for this chunk?
    if (empty) {
        // 4J - added - clear any renderer data associated with this
        for (int currentLayer = 0; currentLayer < 2; currentLayer++) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY0,
                                              currentLayer);
            PlatformRenderer.CBuffClear(lists + currentLayer);
        }

#ifdef OCCLUSION_MODE_BFS
        int globalIdx = levelRenderer->getGlobalIndexForChunk(this->x, this->y,
                                                              this->z, level);
        levelRenderer->setGlobalChunkConnectivity(globalIdx, ~0ULL);
        levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                          LevelRenderer::CHUNK_FLAG_COMPILED);
#endif

        delete region;
        delete tileRenderer;
        return;
    }
    // 4J - optimisation ends
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Tesselator::Bounds bounds;  // 4J MGH - added
    {
        // this was the old default clip bounds for the chunk, set in
        // Chunk::setPos.
        float g = 6.0f;
        bounds.boundingBox[0] = -g;
        bounds.boundingBox[1] = -g;
        bounds.boundingBox[2] = -g;
        bounds.boundingBox[3] = XZSIZE + g;
        bounds.boundingBox[4] = SIZE + g;
        bounds.boundingBox[5] = XZSIZE + g;
    }
    for (int currentLayer = 0; currentLayer < 2; currentLayer++) {
        bool renderNextLayer = false;
        bool rendered = false;

        bool started = false;

        // 4J - changed loop order here to leave y as the innermost loop for
        // better cache performance
        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                for (int y = y0; y < y1; y++) {
                    // 4J Stu - tile data is ordered in 128 blocks of full
                    // width, lower 128 then upper 128
                    int indexY = y;
                    int offset = 0;
                    if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }

                    // 4J - get tile from those copied into our local array in
                    // earlier optimisation
                    unsigned char tileId =
                        tileIds[offset +
                                (((x - x0) << 11) | ((z - z0) << 7) | indexY)];
                    // If flagged as not visible, drop out straight away
                    if (tileId == 0xff) continue;
                    //					int tileId =
                    // region->getTile(x,y,z);
                    if (tileId > 0) {
                        if (!started) {
                            started = true;

                            glNewList(lists + currentLayer, GL_COMPILE);
                            glDepthMask(true);            // 4J added
                            t->useCompactVertices(true);  // 4J added
                            t->begin();
                            t->offset((float)(-this->x), (float)(-this->y),
                                      (float)(-this->z));
                        }

                        Tile* tile = Tile::tiles[tileId];
                        if (currentLayer == 0 && tile->isEntityTile()) {
                            std::shared_ptr<TileEntity> et =
                                region->getTileEntity(x, y, z);
                            if (TileEntityRenderDispatcher::instance
                                    ->hasRenderer(et)) {
                                renderableTileEntities.push_back(et);
                            }
                        }
                        int renderLayer = tile->getRenderLayer();

                        if (renderLayer != currentLayer) {
                            renderNextLayer = true;
                        } else if (renderLayer == currentLayer) {
                            rendered |=
                                tileRenderer->tesselateInWorld(tile, x, y, z);
                        }
                    }
                }
            }
        }

        if (started) {
            t->end();
            bounds.addBounds(t->bounds);  // 4J MGH - added
            glEndList();
            t->useCompactVertices(false);  // 4J added
            t->offset(0, 0, 0);
        } else {
            rendered = false;
        }

        if (rendered) {
            levelRenderer->clearGlobalChunkFlag(
                this->x, this->y, this->z, level,
                LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
        } else {
            // 4J - added - clear any renderer data associated with this unused
            // list
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY0,
                                              currentLayer);
            PlatformRenderer.CBuffClear(lists + currentLayer);
        }
        if ((currentLayer == 0) && (!renderNextLayer)) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY1);
            PlatformRenderer.CBuffClear(lists + 1);
            break;
        }
    }

    // 4J MGH - added this to take the bound from the value calc'd in the
    // tesselator
    bb = {bounds.boundingBox[0], bounds.boundingBox[1], bounds.boundingBox[2],
          bounds.boundingBox[3], bounds.boundingBox[4], bounds.boundingBox[5]};

#ifdef OCCLUSION_MODE_BFS
    uint64_t conn = computeConnectivity(tileIds);  // pass tileIds
    int globalIdx =
        levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level);
    levelRenderer->setGlobalChunkConnectivity(globalIdx, conn);
#endif

    delete tileRenderer;
    delete region;

    // 4J - have rewritten the way that tile entities are stored globally to
    // make it work more easily with split screen. Chunks are now stored
    // globally in the levelrenderer, in a hashmap with a special key made up
    // from the dimension and chunk position (using same index as is used for
    // global flags)
    {
        std::lock_guard<std::mutex> lock(*globalRenderableTileEntities_cs);
        reconcileRenderableTileEntities(renderableTileEntities);
    }

    // 4J - These removed items are now also removed from
    // globalRenderableTileEntities

    if (LevelChunk::touchedSky) {
        levelRenderer->clearGlobalChunkFlag(
            x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    } else {
        levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                          LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    }
    levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                      LevelRenderer::CHUNK_FLAG_COMPILED);

    return;
}

float Chunk::distanceToSqr(std::shared_ptr<Entity> player) const {
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym);
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr(std::shared_ptr<Entity> player) {
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym) * 2;
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

#ifdef OCCLUSION_MODE_BFS
uint64_t Chunk::computeConnectivity(const uint8_t* tileIds) {
    const int W = 16;
    const int H = 16;
    const int VOLUME = W * H * W;

    auto idx = [&](int x, int y, int z) -> int {
        return y * W * W + z * W + x;
    };

    auto isOpen = [&](int lx, int ly, int lz) -> bool {
        int worldY = this->y + ly;
        int offset = 0;
        int indexY = worldY;
        if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
            indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
            offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
        }

        uint8_t tileId = tileIds[offset + ((lx << 11) | (lz << 7) | indexY)];

        if (tileId == 0) return true;      // air
        if (tileId == 0xFF) return false;  // hidden tile (yeah)

        Tile* t = Tile::tiles[tileId];
        return (t == nullptr) || !t->isSolidRender();
    };

    uint8_t visited[6][512];
    memset(visited, 0, sizeof(visited));

    static const int FX[6] = {1, -1, 0, 0, 0, 0};
    static const int FY[6] = {0, 0, 1, -1, 0, 0};
    static const int FZ[6] = {0, 0, 0, 0, 1, -1};

    struct Cell {
        int8_t x, y, z;
    };
    static thread_local std::vector<Cell> queue;

    uint64_t result = 0;

    for (int entryFace = 0; entryFace < 6; entryFace++) {
        uint8_t* vis = visited[entryFace];
        queue.clear();
        int x0s, x1s, y0s, y1s, z0s, z1s;
        switch (entryFace) {
            case 0:
                x0s = W - 1;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // +X
            case 1:
                x0s = 0;
                x1s = 0;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // -X
            case 2:
                x0s = 0;
                x1s = W - 1;
                y0s = H - 1;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // +Y
            case 3:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = 0;
                z0s = 0;
                z1s = W - 1;
                break;  // -Y
            case 4:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = W - 1;
                z1s = W - 1;
                break;  // +Z
            case 5:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = 0;
                break;  // -Z
            default:
                continue;
        }

        for (int sy = y0s; sy <= y1s; sy++)
            for (int sz = z0s; sz <= z1s; sz++)
                for (int sx = x0s; sx <= x1s; sx++) {
                    if (!isOpen(sx, sy, sz)) continue;
                    int i = idx(sx, sy, sz);
                    if (vis[i >> 3] & (1 << (i & 7))) continue;
                    vis[i >> 3] |= (1 << (i & 7));
                    queue.push_back({(int8_t)sx, (int8_t)sy, (int8_t)sz});
                }

        for (int qi = 0; qi < (int)queue.size(); qi++) {
            Cell cur = queue[qi];

            for (int nb = 0; nb < 6; nb++) {
                int nx = cur.x + FX[nb];
                int ny = cur.y + FY[nb];
                int nz = cur.z + FZ[nb];

                // entry exit conn
                if (nx < 0 || nx >= W || ny < 0 || ny >= H || nz < 0 ||
                    nz >= W) {
                    // nb IS the exit face because FX,FY,FZ are aligned
                    result |= ((uint64_t)1 << (entryFace * 6 + nb));
                    continue;
                }

                if (!isOpen(nx, ny, nz)) continue;

                int i = idx(nx, ny, nz);
                if (vis[i >> 3] & (1 << (i & 7))) continue;
                vis[i >> 3] |= (1 << (i & 7));
                queue.push_back({(int8_t)nx, (int8_t)ny, (int8_t)nz});
            }
        }
    }

    return result;
}
#endif

void Chunk::reset() {
    if (assigned) {
        int oldKey = -1;
        bool retireRenderableTileEntities = false;

        {
            std::lock_guard<std::recursive_mutex> lock(
                levelRenderer->m_csDirtyChunks);
            oldKey = levelRenderer->getGlobalIndexForChunk(x, y, z, level);
            unsigned char refCount =
                levelRenderer->decGlobalChunkRefCount(x, y, z, level);
            assigned = false;
            //		printf("\t\t [dec] refcount %d at %d, %d,
            //%d\n",refCount,x,y,z);
            if (refCount == 0 && oldKey != -1) {
                retireRenderableTileEntities = true;
                int lists = oldKey * 2;
                if (lists >= 0) {
                    lists += levelRenderer->chunkLists;
                    for (int i = 0; i < 2; i++) {
                        // 4J - added - clear any renderer data associated with
                        // this unused list
                        PlatformRenderer.CBuffClear(lists + i);
                    }
                    levelRenderer->setGlobalChunkFlags(x, y, z, level, 0);
                }
            }
        }

        if (retireRenderableTileEntities) {
            levelRenderer->retireRenderableTileEntitiesForChunkKey(oldKey);
        }
    }

    clipChunk->visible = false;
}

void Chunk::_delete() {
    reset();
    level = nullptr;
}

int Chunk::getList(int layer) {
    if (!clipChunk->visible) return -1;

    int lists = levelRenderer->getGlobalIndexForChunk(x, y, z, level) * 2;
    lists += levelRenderer->chunkLists;

    bool empty = levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
    if (!empty) return lists + layer;
    return -1;
}

void Chunk::cull(Culler* culler) {
    if (clipChunk->visible) {
        clipChunk->visible = culler->isVisible(&bb);
    }
}

void Chunk::renderBB() {
    //	glCallList(lists + 2);	// 4J - removed - TODO put back in
}

bool Chunk::isEmpty() {
    if (!levelRenderer->getGlobalChunkFlag(x, y, z, level,
                                           LevelRenderer::CHUNK_FLAG_COMPILED))
        return false;
    return levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTYBOTH);
}

void Chunk::setDirty() {
    // 4J - not used, but if this starts being used again then we'll need to
    // investigate how best to handle it.
    assert(0);
    levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                      LevelRenderer::CHUNK_FLAG_DIRTY);
}

void Chunk::clearDirty() {
    levelRenderer->clearGlobalChunkFlag(x, y, z, level,
                                        LevelRenderer::CHUNK_FLAG_DIRTY);
#if defined(_CRITICAL_CHUNKS)
    levelRenderer->clearGlobalChunkFlag(x, y, z, level,
                                        LevelRenderer::CHUNK_FLAG_CRITICAL);
#endif
}

bool Chunk::emptyFlagSet(int layer) {
    return levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
}
