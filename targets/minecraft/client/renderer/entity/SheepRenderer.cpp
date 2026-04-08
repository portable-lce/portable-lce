#include "SheepRenderer.h"
#include "platform/stubs.h"

#include <memory>
#include <string>

#include "platform/renderer/renderer.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/renderer/entity/MobRenderer.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/animal/Sheep.h"

class Model;

ResourceLocation SheepRenderer::SHEEP_LOCATION = ResourceLocation(TN_MOB_SHEEP);
ResourceLocation SheepRenderer::SHEEP_FUR_LOCATION =
    ResourceLocation(TN_MOB_SHEEP_FUR);

SheepRenderer::SheepRenderer(Model* model, Model* armor, float shadow)
    : MobRenderer(model, shadow) {
    setArmor(armor);
}

int SheepRenderer::prepareArmor(std::shared_ptr<LivingEntity> _sheep, int layer,
                                float a) {
    // 4J - dynamic cast required because we aren't using templates/generics in
    // our version
    std::shared_ptr<Sheep> sheep = std::dynamic_pointer_cast<Sheep>(_sheep);

    if (layer == 0 && !sheep->isSheared() &&
        !sheep->isInvisibleTo(
            Minecraft::GetInstance()
                ->player))  // 4J-JEV: Todo, merge with java fix (for invisible
                            // sheep armour) in '1.7.5'.
    {
        bindTexture(&SHEEP_FUR_LOCATION);

        if (sheep->hasCustomName() &&
            sheep->getCustomName().compare("jeb_") == 0) {
            // easter egg...
            int colorDuration = 25;
            int value = (sheep->tickCount / colorDuration) + sheep->entityId;
            int c1 = value % Sheep::COLOR_LENGTH;
            int c2 = (value + 1) % Sheep::COLOR_LENGTH;
            float subStep =
                ((sheep->tickCount % colorDuration) + a) / (float)colorDuration;

            glColor3f(Sheep::COLOR[c1][0] * (1.0f - subStep) +
                          Sheep::COLOR[c2][0] * subStep,
                      Sheep::COLOR[c1][1] * (1.0f - subStep) +
                          Sheep::COLOR[c2][1] * subStep,
                      Sheep::COLOR[c1][2] * (1.0f - subStep) +
                          Sheep::COLOR[c2][2] * subStep);
        } else {
            int color = sheep->getColor();
            glColor3f(Sheep::COLOR[color][0], Sheep::COLOR[color][1],
                      Sheep::COLOR[color][2]);
        }

        // 4J - change brought forward from 1.8.2
        float brightness =
            SharedConstants::TEXTURE_LIGHTING ? 1.0f : sheep->getBrightness(a);
        int color = sheep->getColor();
        glColor3f(brightness * Sheep::COLOR[color][0],
                  brightness * Sheep::COLOR[color][1],
                  brightness * Sheep::COLOR[color][2]);
        return 1;
    }
    return -1;
}

void SheepRenderer::render(std::shared_ptr<Entity> mob, double x, double y,
                           double z, float rot, float a) {
    MobRenderer::render(mob, x, y, z, rot, a);
}

ResourceLocation* SheepRenderer::getTextureLocation(
    std::shared_ptr<Entity> mob) {
    return &SHEEP_LOCATION;
}
