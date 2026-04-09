#include "CaveSpider.h"

#include <memory>

#include "minecraft/SharedConstants.h"
#include "minecraft/world/Difficulty.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/monster/Spider.h"
#include "minecraft/world/level/Level.h"

CaveSpider::CaveSpider(Level* level) : Spider(level) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    registerAttributes();

    this->setSize(0.7f, 0.5f);
}

void CaveSpider::registerAttributes() {
    Spider::registerAttributes();

    getAttribute(SharedMonsterAttributes::MAX_HEALTH)->setBaseValue(12);
}

bool CaveSpider::doHurtTarget(std::shared_ptr<Entity> target) {
    if (Spider::doHurtTarget(target)) {
        if (target->instanceof (eTYPE_LIVINGENTITY)) {
            int poisonTime = 0;
            if (level->difficulty <= Difficulty::EASY) {
                // No poison!
            } else if (level->difficulty == Difficulty::NORMAL) {
                poisonTime = 7;
            } else if (level->difficulty == Difficulty::HARD) {
                poisonTime = 15;
            }

            if (poisonTime > 0) {
                std::dynamic_pointer_cast<LivingEntity>(target)->addEffect(
                    new MobEffectInstance(
                        MobEffect::poison->id,
                        poisonTime * SharedConstants::TICKS_PER_SECOND, 0));
            }
        }

        return true;
    }
    return false;
}

MobGroupData* CaveSpider::finalizeMobSpawn(
    MobGroupData* groupData, int extraData /*= 0*/)  // 4J Added extraData param
{
    // do nothing
    return groupData;
}