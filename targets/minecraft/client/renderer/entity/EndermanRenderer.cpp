#include "EndermanRenderer.h"
#include "platform/stubs.h"

#include <memory>


#include "platform/renderer/renderer.h"

#include "minecraft/SharedConstants.h"
#include "minecraft/client/model/EndermanModel.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/TileRenderer.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/monster/EnderMan.h"
#include "minecraft/world/level/tile/Tile.h"

ResourceLocation EndermanRenderer::ENDERMAN_EYES_LOCATION =
    ResourceLocation(TN_MOB_ENDERMAN_EYES);
ResourceLocation EndermanRenderer::ENDERMAN_LOCATION =
    ResourceLocation(TN_MOB_ENDERMAN);

EndermanRenderer::EndermanRenderer() : MobRenderer(new EndermanModel(), 0.5f) {
    model = (EndermanModel*)MobRenderer::model;
    this->setArmor(model);
}

void EndermanRenderer::render(std::shared_ptr<Entity> _mob, double x, double y,
                              double z, float rot, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Boat rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<EnderMan> mob = std::dynamic_pointer_cast<EnderMan>(_mob);

    model->carrying = mob->getCarryingTile() > 0;
    model->creepy = mob->isCreepy();

    if (mob->isCreepy()) {
        double d = 0.02;
        x += random.nextGaussian() * d;
        z += random.nextGaussian() * d;
    }

    MobRenderer::render(mob, x, y, z, rot, a);
}

ResourceLocation* EndermanRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &ENDERMAN_LOCATION;
}

void EndermanRenderer::additionalRendering(std::shared_ptr<LivingEntity> _mob,
                                           float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Boat rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<EnderMan> mob = std::dynamic_pointer_cast<EnderMan>(_mob);

    MobRenderer::additionalRendering(_mob, a);

    if (mob->getCarryingTile() > 0) {
        glEnable(GL_RESCALE_NORMAL);
        glPushMatrix();

        float s = 8 / 16.0f;
        glTranslatef(-0 / 16.0f, 11 / 16.0f, -12 / 16.0f);
        s *= 1.00f;
        glRotatef(20, 1, 0, 0);
        glRotatef(45, 0, 1, 0);
        glScalef(-s, -s, s);

        if (SharedConstants::TEXTURE_LIGHTING) {
            int col = mob->getLightColor(a);
            int u = col % 65536;
            int v = col / 65536;

            glMultiTexCoord2f(GL_TEXTURE1, u / 1.0f, v / 1.0f);
            glColor4f(1, 1, 1, 1);
        }

        glColor4f(1, 1, 1, 1);
        bindTexture(&TextureAtlas::LOCATION_BLOCKS);  // TODO: bind by icon
        tileRenderer->renderTile(Tile::tiles[mob->getCarryingTile()],
                                 mob->getCarryingData(), 1);
        glPopMatrix();
        glDisable(GL_RESCALE_NORMAL);
    }
}

int EndermanRenderer::prepareArmor(std::shared_ptr<LivingEntity> _mob,
                                   int layer, float a) {
    // 4J - original version used generics and thus had an input parameter of
    // type Boat rather than shared_ptr<Entity>  we have here - do some casting
    // around instead
    std::shared_ptr<EnderMan> mob = std::dynamic_pointer_cast<EnderMan>(_mob);

    if (layer != 0) return -1;

    bindTexture(&ENDERMAN_EYES_LOCATION);  // 4J was "/mob/enderman_eyes.png"
    float br = 1;
    glEnable(GL_BLEND);
    // 4J Stu - We probably don't need to do this on 360 either (as we force it
    // back on the renderer) However we do want it off for other platforms that
    // don't force it on in the render lib CBuff handling Several texture packs
    // have fully transparent bits that break if this is off
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_LIGHTING);

    if (mob->isInvisible()) {
        glDepthMask(false);
    } else {
        glDepthMask(true);
    }

    if (SharedConstants::TEXTURE_LIGHTING) {
        int col = 0xf0f0;
        int u = col % 65536;
        int v = col / 65536;

        glMultiTexCoord2f(GL_TEXTURE1, u / 1.0f, v / 1.0f);
        glColor4f(1, 1, 1, 1);
    }

    glEnable(GL_LIGHTING);
    glColor4f(1, 1, 1, br);
    return 1;
}