#include "TheEndPortalTile.h"

#include <string>

#include "TheEndPortalTileEntity.h"
#include "java/Class.h"
#include "java/Random.h"
#include "minecraft/IGameServices.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/world/IconRegister.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/BaseEntityTile.h"

class Material;

thread_local bool TheEndPortal::m_tlsAllowAnywhere = false;

// 4J - allowAnywhere is a static in java, implementing as TLS here to make
// thread safe
bool TheEndPortal::allowAnywhere() { return m_tlsAllowAnywhere; }

void TheEndPortal::allowAnywhere(bool set) { m_tlsAllowAnywhere = set; }

TheEndPortal::TheEndPortal(int id, Material* material)
    : BaseEntityTile(id, material, false) {
    this->setLightEmission(1.0f);
}

std::shared_ptr<TileEntity> TheEndPortal::newTileEntity(Level* level) {
    return std::make_shared<TheEndPortalTileEntity>();
}

void TheEndPortal::updateShape(
    LevelSource* level, int x, int y, int z, int forceData,
    std::shared_ptr<TileEntity>
        forceEntity)  // 4J added forceData, forceEntity param
{
    float r = 1 / 16.0f;
    setShape(0, 0, 0, 1, r, 1);
}

bool TheEndPortal::shouldRenderFace(LevelSource* level, int x, int y, int z,
                                    int face) {
    if (face != 0) return false;
    return BaseEntityTile::shouldRenderFace(level, x, y, z, face);
}

void TheEndPortal::addAABBs(Level* level, int x, int y, int z, AABB* box,
                            std::vector<AABB>* boxes,
                            std::shared_ptr<Entity> source) {}

bool TheEndPortal::isSolidRender(bool isServerLevel) { return false; }

bool TheEndPortal::isCubeShaped() { return false; }

int TheEndPortal::getResourceCount(Random* random) { return 0; }

void TheEndPortal::entityInside(Level* level, int x, int y, int z,
                                std::shared_ptr<Entity> entity) {
    if (entity->GetType() == eTYPE_EXPERIENCEORB) return;  // 4J added

    if (entity->riding == nullptr && entity->rider.lock() == nullptr) {
        if (!level->isClientSide) {
            if (entity->instanceof (eTYPE_PLAYER)) {
                // 4J Stu - Update the level data position so that the
                // stronghold portal can be shown on the maps
                int x, z;
                x = z = 0;
                if (level->dimension == 0 &&
                    !level->getLevelData()->getHasStrongholdEndPortal() &&
                    gameServices().getTerrainFeaturePosition(
                        eTerrainFeature_StrongholdEndPortal, &x, &z)) {
                    level->getLevelData()->setXStrongholdEndPortal(x);
                    level->getLevelData()->setZStrongholdEndPortal(z);
                    level->getLevelData()->setHasStrongholdEndPortal();
                }
            }
            entity->changeDimension(1);
        }
    }
}

void TheEndPortal::animateTick(Level* level, int xt, int yt, int zt,
                               Random* random) {
    double x = xt + random->nextFloat();
    double y = yt + 0.8f;
    double z = zt + random->nextFloat();
    double xa = 0;
    double ya = 0;
    double za = 0;

    level->addParticle(eParticleType_endportal, x, y, z, xa, ya, za);
}

int TheEndPortal::getRenderShape() { return SHAPE_INVISIBLE; }

void TheEndPortal::onPlace(Level* level, int x, int y, int z) {
    if (allowAnywhere()) return;

    if (level->dimension->id != 0) {
        level->removeTile(x, y, z);
        return;
    }
}

int TheEndPortal::cloneTileId(Level* level, int x, int y, int z) { return 0; }

void TheEndPortal::registerIcons(IconRegister* iconRegister) {
    // don't register null, because of particles
    icon = iconRegister->registerIcon("portal");
}
