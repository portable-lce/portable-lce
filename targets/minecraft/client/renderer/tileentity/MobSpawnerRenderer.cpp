#include "MobSpawnerRenderer.h"

#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/BaseMobSpawner.h"
#include "minecraft/world/level/tile/entity/MobSpawnerTileEntity.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

void MobSpawnerRenderer::render(std::shared_ptr<TileEntity> _spawner, double x,
                                double y, double z, float a, bool setColor,
                                float alpha, bool useCompiled) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<MobSpawnerTileEntity> spawner =
        std::dynamic_pointer_cast<MobSpawnerTileEntity>(_spawner);
    render(spawner->getSpawner(), x, y, z, a);
    glPopMatrix();
}

void MobSpawnerRenderer::render(BaseMobSpawner* spawner, double x, double y,
                                double z, float a) {
    glPushMatrix();
    glTranslatef((float)x + 0.5f, (float)y, (float)z + 0.5f);

    std::shared_ptr<Entity> e = spawner->getDisplayEntity();
    if (e != nullptr) {
        e->setLevel(spawner->getLevel());
        float s = 7 / 16.0f;
        glTranslatef(0, 0.4f, 0);
        glRotatef(
            (float)(spawner->oSpin + (spawner->spin - spawner->oSpin) * a) * 10,
            0, 1, 0);
        glRotatef(-30, 1, 0, 0);
        glTranslatef(0, -0.4f, 0);
        glScalef(s, s, s);
        e->moveTo(x, y, z, 0, 0);
        EntityRenderDispatcher::instance->render(e, 0, 0, 0, 0, a);
    }
}
