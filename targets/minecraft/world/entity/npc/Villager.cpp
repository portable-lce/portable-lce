#include "minecraft/IGameServices.h"
#include "Villager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "Pos.h"
#include "SharedConstants.h"
#include "java/Random.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/AgeableMob.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/EntityEvent.h"
#include "minecraft/world/entity/LivingEntity.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/entity/SyncedEntityData.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/entity/ai/goal/AvoidPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/FloatGoal.h"
#include "minecraft/world/entity/ai/goal/GoalSelector.h"
#include "minecraft/world/entity/ai/goal/InteractGoal.h"
#include "minecraft/world/entity/ai/goal/LookAtPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/LookAtTradingPlayerGoal.h"
#include "minecraft/world/entity/ai/goal/MakeLoveGoal.h"
#include "minecraft/world/entity/ai/goal/MoveIndoorsGoal.h"
#include "minecraft/world/entity/ai/goal/MoveTowardsRestrictionGoal.h"
#include "minecraft/world/entity/ai/goal/OpenDoorGoal.h"
#include "minecraft/world/entity/ai/goal/PlayGoal.h"
#include "minecraft/world/entity/ai/goal/RandomStrollGoal.h"
#include "minecraft/world/entity/ai/goal/RestrictOpenDoorGoal.h"
#include "minecraft/world/entity/ai/goal/TakeFlowerGoal.h"
#include "minecraft/world/entity/ai/goal/TradeWithPlayerGoal.h"
#include "minecraft/world/entity/ai/navigation/PathNavigation.h"
#include "minecraft/world/entity/ai/village/Village.h"
#include "minecraft/world/entity/ai/village/Villages.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/monster/Zombie.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/item/EnchantedBookItem.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/item/enchantment/Enchantment.h"
#include "minecraft/world/item/enchantment/EnchantmentHelper.h"
#include "minecraft/world/item/enchantment/EnchantmentInstance.h"
#include "minecraft/world/item/trading/Merchant.h"
#include "minecraft/world/item/trading/MerchantRecipe.h"
#include "minecraft/world/item/trading/MerchantRecipeList.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/tile/Tile.h"
#include "nbt/CompoundTag.h"
#include "strings.h"

namespace {
struct VillagerShuffleRandom {
    using result_type = unsigned int;

    explicit VillagerShuffleRandom(Random* random) : random(random) {}

    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return 0xFFFFFFFFu; }

    result_type operator()() {
        return static_cast<result_type>(random->nextInt());
    }

    Random* random;
};
}  // namespace

std::unordered_map<int, std::pair<int, int> > Villager::MIN_MAX_VALUES;
std::unordered_map<int, std::pair<int, int> > Villager::MIN_MAX_PRICES;

void Villager::_init(int profession) {
    // 4J Stu - This function call had to be moved here from the Entity ctor to
    // ensure that the derived version of the function is called
    this->defineSynchedData();
    registerAttributes();
    setHealth(getMaxHealth());

    setProfession(profession);
    setSize(.6f, 1.8f);

    villageUpdateInterval = 0;
    inLove = false;
    chasing = false;
    village = std::weak_ptr<Village>();

    tradingPlayer = std::weak_ptr<Player>();
    offers = nullptr;
    updateMerchantTimer = 0;
    addRecipeOnUpdate = false;
    riches = 0;
    lastPlayerTradeName = "";
    rewardPlayersOnFirstVillage = false;
    baseRecipeChanceMod = 0.0f;

    getNavigation()->setCanOpenDoors(true);
    getNavigation()->setAvoidWater(true);

    goalSelector.addGoal(0, new FloatGoal(this));
    goalSelector.addGoal(
        1, new AvoidPlayerGoal(this, typeid(Zombie), 8, 0.6, 0.6));
    goalSelector.addGoal(1, new TradeWithPlayerGoal(this));
    goalSelector.addGoal(1, new LookAtTradingPlayerGoal(this));
    goalSelector.addGoal(2, new MoveIndoorsGoal(this));
    goalSelector.addGoal(3, new RestrictOpenDoorGoal(this));
    goalSelector.addGoal(4, new OpenDoorGoal(this, true));
    goalSelector.addGoal(5, new MoveTowardsRestrictionGoal(this, 0.6));
    goalSelector.addGoal(6, new MakeLoveGoal(this));
    goalSelector.addGoal(7, new TakeFlowerGoal(this));
    goalSelector.addGoal(8, new PlayGoal(this, 0.32));
    goalSelector.addGoal(9, new InteractGoal(this, typeid(Player), 3, 1.f));
    goalSelector.addGoal(9, new InteractGoal(this, typeid(Villager), 5, 0.02f));
    goalSelector.addGoal(9, new RandomStrollGoal(this, 0.6));
    goalSelector.addGoal(10, new LookAtPlayerGoal(this, typeid(Mob), 8));
}

Villager::Villager(Level* level) : AgableMob(level) { _init(0); }

Villager::Villager(Level* level, int profession) : AgableMob(level) {
    _init(profession);
}

Villager::~Villager() { delete offers; }

void Villager::registerAttributes() {
    AgableMob::registerAttributes();

    getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED)->setBaseValue(0.5f);
}

bool Villager::useNewAi() { return true; }

void Villager::serverAiMobStep() {
    if (--villageUpdateInterval <= 0) {
        level->villages->queryUpdateAround(Mth::floor(x), Mth::floor(y),
                                           Mth::floor(z));
        villageUpdateInterval = 70 + random->nextInt(50);

        std::shared_ptr<Village> _village = level->villages->getClosestVillage(
            Mth::floor(x), Mth::floor(y), Mth::floor(z), Villages::MaxDoorDist);
        village = _village;
        if (_village == nullptr)
            clearRestriction();
        else {
            Pos* center = _village->getCenter();
            restrictTo(center->x, center->y, center->z,
                       (int)((float)_village->getRadius() * 0.6f));
            if (rewardPlayersOnFirstVillage) {
                rewardPlayersOnFirstVillage = false;
                _village->rewardAllPlayers(5);
            }
        }
    }

    if (!isTrading() && updateMerchantTimer > 0) {
        updateMerchantTimer--;
        if (updateMerchantTimer <= 0) {
            if (addRecipeOnUpdate) {
                // improve max uses for all obsolete recipes
                if (offers->size() > 0) {
                    // for (MerchantRecipe recipe : offers)
                    for (auto it = offers->begin(); it != offers->end(); ++it) {
                        MerchantRecipe* recipe = *it;
                        if (recipe->isDeprecated()) {
                            recipe->increaseMaxUses(random->nextInt(6) +
                                                    random->nextInt(6) + 2);
                        }
                    }
                }
                addOffers(1);
                addRecipeOnUpdate = false;

                if (village.lock() != nullptr && !lastPlayerTradeName.empty()) {
                    level->broadcastEntityEvent(shared_from_this(),
                                                EntityEvent::VILLAGER_HAPPY);
                    village.lock()->modifyStanding(lastPlayerTradeName, 1);
                }
            }
            addEffect(new MobEffectInstance(
                MobEffect::regeneration->id,
                SharedConstants::TICKS_PER_SECOND * 10, 0));
        }
    }

    AgableMob::serverAiMobStep();
}

bool Villager::mobInteract(std::shared_ptr<Player> player) {
    // [EB]: Truly dislike this code but I don't see another easy way
    std::shared_ptr<ItemInstance> item = player->inventory->getSelected();
    bool holdingSpawnEgg = item != nullptr && item->id == Item::spawnEgg_Id;

    if (!holdingSpawnEgg && isAlive() && !isTrading() && !isBaby()) {
        if (!level->isClientSide) {
            // note: stop() logic is controlled by trading ai goal
            setTradingPlayer(player);

            // 4J-JEV: Villagers in PC game don't display professions.
            player->openTrading(
                std::dynamic_pointer_cast<Merchant>(shared_from_this()),
                getDisplayName());
        }
        return true;
    }
    return AgableMob::mobInteract(player);
}

void Villager::defineSynchedData() {
    AgableMob::defineSynchedData();
    entityData->define(DATA_PROFESSION_ID, 0);
}

void Villager::addAdditonalSaveData(CompoundTag* tag) {
    AgableMob::addAdditonalSaveData(tag);
    tag->putInt("Profession", getProfession());
    tag->putInt("Riches", riches);
    if (offers != nullptr) {
        tag->putCompound("Offers", offers->createTag());
    }
}

void Villager::readAdditionalSaveData(CompoundTag* tag) {
    AgableMob::readAdditionalSaveData(tag);
    setProfession(tag->getInt("Profession"));
    riches = tag->getInt("Riches");
    if (tag->contains("Offers")) {
        CompoundTag* compound = tag->getCompound("Offers");
        delete offers;
        offers = new MerchantRecipeList(compound);
    }
}

bool Villager::removeWhenFarAway() { return false; }

int Villager::getAmbientSound() {
    if (isTrading()) {
        return eSoundType_MOB_VILLAGER_HAGGLE;
    }
    return eSoundType_MOB_VILLAGER_IDLE;
}

int Villager::getHurtSound() { return eSoundType_MOB_VILLAGER_HIT; }

int Villager::getDeathSound() { return eSoundType_MOB_VILLAGER_DEATH; }

void Villager::setProfession(int profession) {
    entityData->set(DATA_PROFESSION_ID, profession);
}

int Villager::getProfession() {
    return entityData->getInteger(DATA_PROFESSION_ID);
}

bool Villager::isInLove() { return inLove; }

void Villager::setInLove(bool inLove) { this->inLove = inLove; }

void Villager::setChasing(bool chasing) { this->chasing = chasing; }

bool Villager::isChasing() { return chasing; }

void Villager::setLastHurtByMob(std::shared_ptr<LivingEntity> mob) {
    AgableMob::setLastHurtByMob(mob);
    std::shared_ptr<Village> _village = village.lock();
    if (_village != nullptr && mob != nullptr) {
        _village->addAggressor(mob);

        if (mob->instanceof(eTYPE_PLAYER)) {
            int amount = -1;
            if (isBaby()) {
                amount = -3;
            }
            _village->modifyStanding(
                std::dynamic_pointer_cast<Player>(mob)->getName(), amount);
            if (isAlive()) {
                level->broadcastEntityEvent(shared_from_this(),
                                            EntityEvent::VILLAGER_ANGRY);
            }
        }
    }
}

void Villager::die(DamageSource* source) {
    std::shared_ptr<Village> _village = village.lock();
    if (_village != nullptr) {
        std::shared_ptr<Entity> sourceEntity = source->getEntity();
        if (sourceEntity != nullptr) {
            if (sourceEntity->instanceof(eTYPE_PLAYER)) {
                _village->modifyStanding(
                    std::dynamic_pointer_cast<Player>(sourceEntity)->getName(),
                    -2);
            } else if (sourceEntity->instanceof(eTYPE_ENEMY)) {
                _village->resetNoBreedTimer();
            }
        } else if (sourceEntity == nullptr) {
            // if the villager was killed by the world (such as lava or
            // falling), blame the nearest player by not reproducing for a while
            std::shared_ptr<Player> nearestPlayer =
                level->getNearestPlayer(shared_from_this(), 16.0f);
            if (nearestPlayer != nullptr) {
                _village->resetNoBreedTimer();
            }
        }
    }

    AgableMob::die(source);
}

void Villager::setTradingPlayer(std::shared_ptr<Player> player) {
    tradingPlayer = std::weak_ptr<Player>(player);
}

std::shared_ptr<Player> Villager::getTradingPlayer() {
    return tradingPlayer.lock();
}

bool Villager::isTrading() { return tradingPlayer.lock() != nullptr; }

void Villager::notifyTrade(MerchantRecipe* activeRecipe) {
    activeRecipe->increaseUses();
    ambientSoundTime = -getAmbientSoundInterval();
    playSound(eSoundType_MOB_VILLAGER_YES, getSoundVolume(), getVoicePitch());

    // when the player buys the latest item, we improve the merchant a little
    // while later
    if (activeRecipe->isSame(offers->at(offers->size() - 1))) {
        updateMerchantTimer = SharedConstants::TICKS_PER_SECOND * 2;
        addRecipeOnUpdate = true;
        if (tradingPlayer.lock() != nullptr) {
            lastPlayerTradeName = tradingPlayer.lock()->getName();
        } else {
            lastPlayerTradeName = "";
        }
    }

    if (activeRecipe->getBuyAItem()->id == Item::emerald_Id) {
        riches += activeRecipe->getBuyAItem()->count;
    }
}

void Villager::notifyTradeUpdated(std::shared_ptr<ItemInstance> item) {
    if (!level->isClientSide &&
        (ambientSoundTime >
         (-getAmbientSoundInterval() + SharedConstants::TICKS_PER_SECOND))) {
        ambientSoundTime = -getAmbientSoundInterval();
        if (item != nullptr) {
            playSound(eSoundType_MOB_VILLAGER_YES, getSoundVolume(),
                      getVoicePitch());
        } else {
            playSound(eSoundType_MOB_VILLAGER_NO, getSoundVolume(),
                      getVoicePitch());
        }
    }
}

MerchantRecipeList* Villager::getOffers(std::shared_ptr<Player> forPlayer) {
    if (offers == nullptr) {
        addOffers(1);
    }
    return offers;
}

float Villager::getRecipeChance(float baseChance) {
    float newChance = baseChance + baseRecipeChanceMod;
    if (newChance > .9f) {
        return .9f - (newChance - .9f);
    }
    return newChance;
}

void Villager::addOffers(int addCount) {
    MerchantRecipeList* newOffers = new MerchantRecipeList();
    switch (getProfession()) {
        case PROFESSION_FARMER:
            addItemForTradeIn(newOffers, Item::wheat_Id, random,
                              getRecipeChance(.9f));
            addItemForTradeIn(newOffers, Tile::wool_Id, random,
                              getRecipeChance(.5f));
            addItemForTradeIn(newOffers, Item::chicken_raw_Id, random,
                              getRecipeChance(.5f));
            addItemForTradeIn(newOffers, Item::fish_cooked_Id, random,
                              getRecipeChance(.4f));
            addItemForPurchase(newOffers, Item::bread_Id, random,
                               getRecipeChance(.9f));
            addItemForPurchase(newOffers, Item::melon_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::apple_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::cookie_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::shears_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::flintAndSteel_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::chicken_cooked_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::arrow_Id, random,
                               getRecipeChance(.5f));
            if (random->nextFloat() < getRecipeChance(.5f)) {
                newOffers->push_back(new MerchantRecipe(
                    std::shared_ptr<ItemInstance>(
                        new ItemInstance(Tile::gravel, 10)),
                    std::shared_ptr<ItemInstance>(
                        new ItemInstance(Item::emerald)),
                    std::make_shared<ItemInstance>(Item::flint_Id,
                                                   4 + random->nextInt(2), 0)));
            }
            break;
        case PROFESSION_BUTCHER:
            addItemForTradeIn(newOffers, Item::coal_Id, random,
                              getRecipeChance(.7f));
            addItemForTradeIn(newOffers, Item::porkChop_raw_Id, random,
                              getRecipeChance(.5f));
            addItemForTradeIn(newOffers, Item::beef_raw_Id, random,
                              getRecipeChance(.5f));
            addItemForPurchase(newOffers, Item::saddle_Id, random,
                               getRecipeChance(.1f));
            addItemForPurchase(newOffers, Item::chestplate_leather_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::boots_leather_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::helmet_leather_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::leggings_leather_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::porkChop_cooked_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::beef_cooked_Id, random,
                               getRecipeChance(.3f));
            break;
        case PROFESSION_SMITH:
            addItemForTradeIn(newOffers, Item::coal_Id, random,
                              getRecipeChance(.7f));
            addItemForTradeIn(newOffers, Item::ironIngot_Id, random,
                              getRecipeChance(.5f));
            addItemForTradeIn(newOffers, Item::goldIngot_Id, random,
                              getRecipeChance(.5f));
            addItemForTradeIn(newOffers, Item::diamond_Id, random,
                              getRecipeChance(.5f));

            addItemForPurchase(newOffers, Item::sword_iron_Id, random,
                               getRecipeChance(.5f));
            addItemForPurchase(newOffers, Item::sword_diamond_Id, random,
                               getRecipeChance(.5f));
            addItemForPurchase(newOffers, Item::hatchet_iron_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::hatchet_diamond_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::pickAxe_iron_Id, random,
                               getRecipeChance(.5f));
            addItemForPurchase(newOffers, Item::pickAxe_diamond_Id, random,
                               getRecipeChance(.5f));
            addItemForPurchase(newOffers, Item::shovel_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::shovel_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::hoe_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::hoe_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::boots_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::boots_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::helmet_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::helmet_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::chestplate_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::chestplate_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::leggings_iron_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::leggings_diamond_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::boots_chain_Id, random,
                               getRecipeChance(.1f));
            addItemForPurchase(newOffers, Item::helmet_chain_Id, random,
                               getRecipeChance(.1f));
            addItemForPurchase(newOffers, Item::chestplate_chain_Id, random,
                               getRecipeChance(.1f));
            addItemForPurchase(newOffers, Item::leggings_chain_Id, random,
                               getRecipeChance(.1f));
            break;
        case PROFESSION_LIBRARIAN:
            addItemForTradeIn(newOffers, Item::paper_Id, random,
                              getRecipeChance(.8f));
            addItemForTradeIn(newOffers, Item::book_Id, random,
                              getRecipeChance(.8f));
            // addItemForTradeIn(newOffers, Item::writtenBook_Id, random,
            // getRecipeChance(0.3f));
            addItemForPurchase(newOffers, Tile::bookshelf_Id, random,
                               getRecipeChance(.8f));
            addItemForPurchase(newOffers, Tile::glass_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::compass_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::clock_Id, random,
                               getRecipeChance(.2f));

            if (random->nextFloat() < getRecipeChance(0.07f)) {
                Enchantment* enchantment =
                    Enchantment::validEnchantments[random->nextInt(
                        Enchantment::validEnchantments.size())];
                int level = random->nextInt(enchantment->getMinLevel(),
                                            enchantment->getMaxLevel());
                std::shared_ptr<ItemInstance> book =
                    Item::enchantedBook->createForEnchantment(
                        new EnchantmentInstance(enchantment, level));
                int cost = 2 + random->nextInt(5 + (level * 10)) + 3 * level;

                newOffers->push_back(new MerchantRecipe(
                    std::make_shared<ItemInstance>(Item::book),
                    std::shared_ptr<ItemInstance>(
                        new ItemInstance(Item::emerald, cost)),
                    book));
            }
            break;
        case PROFESSION_PRIEST:
            addItemForPurchase(newOffers, Item::eyeOfEnder_Id, random,
                               getRecipeChance(.3f));
            addItemForPurchase(newOffers, Item::expBottle_Id, random,
                               getRecipeChance(.2f));
            addItemForPurchase(newOffers, Item::redStone_Id, random,
                               getRecipeChance(.4f));
            addItemForPurchase(newOffers, Tile::glowstone_Id, random,
                               getRecipeChance(.3f));
            {
                int enchantItems[] = {
                    Item::sword_iron_Id,      Item::sword_diamond_Id,
                    Item::chestplate_iron_Id, Item::chestplate_diamond_Id,
                    Item::hatchet_iron_Id,    Item::hatchet_diamond_Id,
                    Item::pickAxe_iron_Id,    Item::pickAxe_diamond_Id};
                for (unsigned int i = 0; i < 8; ++i) {
                    int id = enchantItems[i];
                    if (random->nextFloat() < getRecipeChance(.05f)) {
                        newOffers->push_back(new MerchantRecipe(
                            std::shared_ptr<ItemInstance>(
                                new ItemInstance(id, 1, 0)),
                            std::make_shared<ItemInstance>(
                                Item::emerald, 2 + random->nextInt(3), 0),
                            EnchantmentHelper::enchantItem(
                                random,
                                std::shared_ptr<ItemInstance>(
                                    new ItemInstance(id, 1, 0)),
                                5 + random->nextInt(15))));
                    }
                }
            }
            break;
    }

    if (newOffers->empty()) {
        addItemForTradeIn(newOffers, Item::goldIngot_Id, random, 1.0f);
    }

    // shuffle the list to make it more interesting
    std::shuffle(newOffers->begin(), newOffers->end(),
                 VillagerShuffleRandom(random));

    if (offers == nullptr) {
        offers = new MerchantRecipeList();
    }
    for (int i = 0; i < addCount && i < newOffers->size(); i++) {
        if (offers->addIfNewOrBetter(newOffers->at(i))) {
            // 4J Added so we can delete newOffers
            newOffers->erase(newOffers->begin() + i);
        }
    }
    delete newOffers;
}

void Villager::overrideOffers(MerchantRecipeList* recipeList) {}

void Villager::staticCtor() {
    MIN_MAX_VALUES[Item::coal_Id] = std::pair<int, int>(16, 24);
    MIN_MAX_VALUES[Item::ironIngot_Id] = std::pair<int, int>(8, 10);
    MIN_MAX_VALUES[Item::goldIngot_Id] = std::pair<int, int>(8, 10);
    MIN_MAX_VALUES[Item::diamond_Id] = std::pair<int, int>(4, 6);
    MIN_MAX_VALUES[Item::paper_Id] = std::pair<int, int>(24, 36);
    MIN_MAX_VALUES[Item::book_Id] = std::pair<int, int>(11, 13);
    // MIN_MAX_VALUES.insert(Item::writtenBook_Id, pair<int,int>(1, 1));
    MIN_MAX_VALUES[Item::enderPearl_Id] = std::pair<int, int>(3, 4);
    MIN_MAX_VALUES[Item::eyeOfEnder_Id] = std::pair<int, int>(2, 3);
    MIN_MAX_VALUES[Item::porkChop_raw_Id] = std::pair<int, int>(14, 18);
    MIN_MAX_VALUES[Item::beef_raw_Id] = std::pair<int, int>(14, 18);
    MIN_MAX_VALUES[Item::chicken_raw_Id] = std::pair<int, int>(14, 18);
    MIN_MAX_VALUES[Item::fish_cooked_Id] = std::pair<int, int>(9, 13);
    MIN_MAX_VALUES[Item::seeds_wheat_Id] = std::pair<int, int>(34, 48);
    MIN_MAX_VALUES[Item::seeds_melon_Id] = std::pair<int, int>(30, 38);
    MIN_MAX_VALUES[Item::seeds_pumpkin_Id] = std::pair<int, int>(30, 38);
    MIN_MAX_VALUES[Item::wheat_Id] = std::pair<int, int>(18, 22);
    MIN_MAX_VALUES[Tile::wool_Id] = std::pair<int, int>(14, 22);
    MIN_MAX_VALUES[Item::rotten_flesh_Id] = std::pair<int, int>(36, 64);

    MIN_MAX_PRICES[Item::flintAndSteel_Id] = std::pair<int, int>(3, 4);
    MIN_MAX_PRICES[Item::shears_Id] = std::pair<int, int>(3, 4);
    MIN_MAX_PRICES[Item::sword_iron_Id] = std::pair<int, int>(7, 11);
    MIN_MAX_PRICES[Item::sword_diamond_Id] = std::pair<int, int>(12, 14);
    MIN_MAX_PRICES[Item::hatchet_iron_Id] = std::pair<int, int>(6, 8);
    MIN_MAX_PRICES[Item::hatchet_diamond_Id] = std::pair<int, int>(9, 12);
    MIN_MAX_PRICES[Item::pickAxe_iron_Id] = std::pair<int, int>(7, 9);
    MIN_MAX_PRICES[Item::pickAxe_diamond_Id] = std::pair<int, int>(10, 12);
    MIN_MAX_PRICES[Item::shovel_iron_Id] = std::pair<int, int>(4, 6);
    MIN_MAX_PRICES[Item::shovel_diamond_Id] = std::pair<int, int>(7, 8);
    MIN_MAX_PRICES[Item::hoe_iron_Id] = std::pair<int, int>(4, 6);
    MIN_MAX_PRICES[Item::hoe_diamond_Id] = std::pair<int, int>(7, 8);
    MIN_MAX_PRICES[Item::boots_iron_Id] = std::pair<int, int>(4, 6);
    MIN_MAX_PRICES[Item::boots_diamond_Id] = std::pair<int, int>(7, 8);
    MIN_MAX_PRICES[Item::helmet_iron_Id] = std::pair<int, int>(4, 6);
    MIN_MAX_PRICES[Item::helmet_diamond_Id] = std::pair<int, int>(7, 8);
    MIN_MAX_PRICES[Item::chestplate_iron_Id] = std::pair<int, int>(10, 14);
    MIN_MAX_PRICES[Item::chestplate_diamond_Id] = std::pair<int, int>(16, 19);
    MIN_MAX_PRICES[Item::leggings_iron_Id] = std::pair<int, int>(8, 10);
    MIN_MAX_PRICES[Item::leggings_diamond_Id] = std::pair<int, int>(11, 14);
    MIN_MAX_PRICES[Item::boots_chain_Id] = std::pair<int, int>(5, 7);
    MIN_MAX_PRICES[Item::helmet_chain_Id] = std::pair<int, int>(5, 7);
    MIN_MAX_PRICES[Item::chestplate_chain_Id] = std::pair<int, int>(11, 15);
    MIN_MAX_PRICES[Item::leggings_chain_Id] = std::pair<int, int>(9, 11);
    MIN_MAX_PRICES[Item::bread_Id] = std::pair<int, int>(-4, -2);
    MIN_MAX_PRICES[Item::melon_Id] = std::pair<int, int>(-8, -4);
    MIN_MAX_PRICES[Item::apple_Id] = std::pair<int, int>(-8, -4);
    MIN_MAX_PRICES[Item::cookie_Id] = std::pair<int, int>(-10, -7);
    MIN_MAX_PRICES[Tile::glass_Id] = std::pair<int, int>(-5, -3);
    MIN_MAX_PRICES[Tile::bookshelf_Id] = std::pair<int, int>(3, 4);
    MIN_MAX_PRICES[Item::chestplate_leather_Id] = std::pair<int, int>(4, 5);
    MIN_MAX_PRICES[Item::boots_leather_Id] = std::pair<int, int>(2, 4);
    MIN_MAX_PRICES[Item::helmet_leather_Id] = std::pair<int, int>(2, 4);
    MIN_MAX_PRICES[Item::leggings_leather_Id] = std::pair<int, int>(2, 4);
    MIN_MAX_PRICES[Item::saddle_Id] = std::pair<int, int>(6, 8);
    MIN_MAX_PRICES[Item::expBottle_Id] = std::pair<int, int>(-4, -1);
    MIN_MAX_PRICES[Item::redStone_Id] = std::pair<int, int>(-4, -1);
    MIN_MAX_PRICES[Item::compass_Id] = std::pair<int, int>(10, 12);
    MIN_MAX_PRICES[Item::clock_Id] = std::pair<int, int>(10, 12);
    MIN_MAX_PRICES[Tile::glowstone_Id] = std::pair<int, int>(-3, -1);
    MIN_MAX_PRICES[Item::porkChop_cooked_Id] = std::pair<int, int>(-7, -5);
    MIN_MAX_PRICES[Item::beef_cooked_Id] = std::pair<int, int>(-7, -5);
    MIN_MAX_PRICES[Item::chicken_cooked_Id] = std::pair<int, int>(-8, -6);
    MIN_MAX_PRICES[Item::eyeOfEnder_Id] = std::pair<int, int>(7, 11);
    MIN_MAX_PRICES[Item::arrow_Id] = std::pair<int, int>(-12, -8);
}

/**
 * Adds a merchant recipe that trades items for a single ruby.
 *
 * @param list
 * @param itemId
 * @param random
 * @param likelyHood
 */
void Villager::addItemForTradeIn(MerchantRecipeList* list, int itemId,
                                 Random* random, float likelyHood) {
    if (random->nextFloat() < likelyHood) {
        list->push_back(new MerchantRecipe(getItemTradeInValue(itemId, random),
                                           Item::emerald));
    }
}

std::shared_ptr<ItemInstance> Villager::getItemTradeInValue(int itemId,
                                                            Random* random) {
    return std::shared_ptr<ItemInstance>(
        new ItemInstance(itemId, getTradeInValue(itemId, random), 0));
}

int Villager::getTradeInValue(int itemId, Random* random) {
    auto it = MIN_MAX_VALUES.find(itemId);
    if (it == MIN_MAX_VALUES.end()) {
        return 1;
    }
    std::pair<int, int> minMax = it->second;
    if (minMax.first >= minMax.second) {
        return minMax.first;
    }
    return minMax.first + random->nextInt(minMax.second - minMax.first);
}

/**
 * Adds a merchant recipe that trades rubies for an item. If the cost is
 * negative, one ruby will give several of that item.
 *
 * @param list
 * @param itemId
 * @param random
 * @param likelyHood
 */
void Villager::addItemForPurchase(MerchantRecipeList* list, int itemId,
                                  Random* random, float likelyHood) {
    if (random->nextFloat() < likelyHood) {
        int purchaseCost = getPurchaseCost(itemId, random);
        std::shared_ptr<ItemInstance> rubyItem;
        std::shared_ptr<ItemInstance> resultItem;
        if (purchaseCost < 0) {
            rubyItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::emerald_Id, 1, 0));
            resultItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(itemId, -purchaseCost, 0));
        } else {
            rubyItem = std::shared_ptr<ItemInstance>(
                new ItemInstance(Item::emerald_Id, purchaseCost, 0));
            resultItem = std::make_shared<ItemInstance>(itemId, 1, 0);
        }
        list->push_back(new MerchantRecipe(rubyItem, resultItem));
    }
}

int Villager::getPurchaseCost(int itemId, Random* random) {
    auto it = MIN_MAX_PRICES.find(itemId);
    if (it == MIN_MAX_PRICES.end()) {
        return 1;
    }
    std::pair<int, int> minMax = it->second;
    if (minMax.first >= minMax.second) {
        return minMax.first;
    }
    return minMax.first + random->nextInt(minMax.second - minMax.first);
}

void Villager::handleEntityEvent(uint8_t id) {
    if (id == EntityEvent::LOVE_HEARTS) {
        addParticlesAroundSelf(eParticleType_heart);
    } else if (id == EntityEvent::VILLAGER_ANGRY) {
        addParticlesAroundSelf(eParticleType_angryVillager);
    } else if (id == EntityEvent::VILLAGER_HAPPY) {
        addParticlesAroundSelf(eParticleType_happyVillager);
    } else {
        AgableMob::handleEntityEvent(id);
    }
}

void Villager::addParticlesAroundSelf(ePARTICLE_TYPE particle) {
    for (int i = 0; i < 5; i++) {
        double xa = random->nextGaussian() * 0.02;
        double ya = random->nextGaussian() * 0.02;
        double za = random->nextGaussian() * 0.02;
        level->addParticle(
            particle, x + random->nextFloat() * bbWidth * 2 - bbWidth,
            y + 1.0f + random->nextFloat() * bbHeight,
            z + random->nextFloat() * bbWidth * 2 - bbWidth, xa, ya, za);
    }
}

MobGroupData* Villager::finalizeMobSpawn(
    MobGroupData* groupData, int extraData /*= 0*/)  // 4J Added extraData param
{
    groupData = AgableMob::finalizeMobSpawn(groupData);

    setProfession(level->random->nextInt(PROFESSION_MAX));

    return groupData;
}

void Villager::setRewardPlayersInVillage() {
    rewardPlayersOnFirstVillage = true;
}

std::shared_ptr<AgableMob> Villager::getBreedOffspring(
    std::shared_ptr<AgableMob> target) {
    // 4J - added limit to villagers that can be bred
    if (level->canCreateMore(GetType(), Level::eSpawnType_Breed)) {
        std::shared_ptr<Villager> villager = std::make_shared<Villager>(level);
        villager->finalizeMobSpawn(nullptr);
        return villager;
    } else {
        return nullptr;
    }
}

bool Villager::canBeLeashed() { return false; }

std::string Villager::getDisplayName() {
    if (hasCustomName()) return getCustomName();

    int name = IDS_VILLAGER;
    switch (getProfession()) {
        case PROFESSION_FARMER:
            name = IDS_VILLAGER_FARMER;
            break;
        case PROFESSION_LIBRARIAN:
            name = IDS_VILLAGER_LIBRARIAN;
            break;
        case PROFESSION_PRIEST:
            name = IDS_VILLAGER_PRIEST;
            break;
        case PROFESSION_SMITH:
            name = IDS_VILLAGER_SMITH;
            break;
        case PROFESSION_BUTCHER:
            name = IDS_VILLAGER_BUTCHER;
            break;
    };
    return gameServices().getString(name);
}
