#include "LocalPlayer.h"

#include <stdio.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <numbers>

#include "Input.h"
#include "java/Random.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/IMenuService.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/User.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/particle/CritParticle.h"
#include "minecraft/client/particle/ParticleEngine.h"
#include "minecraft/client/particle/TakeAnimationParticle.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/renderer/ItemInHandRenderer.h"
#include "minecraft/stats/StatsCounter.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/entity/ai/attributes/AttributeInstance.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
// 4J : WESTY : Added for new achievements.
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/Tile.h"
// 4J Stu - Added for tutorial callbacks
#include "PlatformTypes.h"
#include "Pos.h"
#include "app/common/App_structs.h"
#include "minecraft/sounds/ConsoleSoundEngine.h"
#include "app/common/Tutorial/Tutorial.h"
#include "app/common/Tutorial/TutorialMode.h"
#include "app/linux/Linux_UIController.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/gui/Gui.h"
#include "minecraft/client/gui/achievement/AchievementPopup.h"
#include "minecraft/client/gui/inventory/BeaconScreen.h"
#include "minecraft/client/gui/inventory/BrewingStandScreen.h"
#include "minecraft/client/gui/inventory/ContainerScreen.h"
#include "minecraft/client/gui/inventory/CraftingScreen.h"
#include "minecraft/client/gui/inventory/EnchantmentScreen.h"
#include "minecraft/client/gui/inventory/FurnaceScreen.h"
#include "minecraft/client/gui/inventory/HopperScreen.h"
#include "minecraft/client/gui/inventory/HorseInventoryScreen.h"
#include "minecraft/client/gui/inventory/MerchantScreen.h"
#include "minecraft/client/gui/inventory/RepairScreen.h"
#include "minecraft/client/gui/inventory/TextEditScreen.h"
#include "minecraft/client/gui/inventory/TrapScreen.h"
#include "minecraft/client/multiplayer/MultiPlayerGameMode.h"
#include "minecraft/commands/CommandsEnum.h"
#include "minecraft/core/particles/ParticleTypes.h"
#include "minecraft/network/INetworkService.h"
#include "minecraft/sounds/SoundTypes.h"
#include "minecraft/stats/Achievement.h"
#include "minecraft/stats/CommonStats.h"
#include "minecraft/stats/GenericStats.h"
#include "minecraft/stats/Stat.h"
#include "minecraft/util/SmoothFloat.h"
#include "minecraft/world/damageSource/DamageSource.h"
#include "minecraft/world/effect/MobEffect.h"
#include "minecraft/world/effect/MobEffectInstance.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/monster/SharedMonsterAttributes.h"
#include "minecraft/world/entity/player/Abilities.h"
#include "minecraft/world/entity/player/Inventory.h"
#include "minecraft/world/entity/player/Player.h"
#include "minecraft/world/food/FoodConstants.h"
#include "minecraft/world/food/FoodData.h"
#include "minecraft/world/item/BowItem.h"
#include "minecraft/world/item/ItemInstance.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/level/tile/entity/CommandBlockEntity.h"
#include "minecraft/world/level/tile/entity/HopperTileEntity.h"
#include "minecraft/world/level/tile/entity/SignTileEntity.h"
#include "minecraft/world/phys/AABB.h"
#include "minecraft/world/phys/HitResult.h"
#include "minecraft/world/phys/Vec3.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#include "platform/renderer/renderer.h"

LocalPlayer::LocalPlayer(Minecraft* minecraft, Level* level, User* user,
                         int dimension)
    : Player(level, user->name) {
    flyX = flyY = flyZ = 0.0f;  // 4J added
    m_awardedThisSession = 0;

    sprintTriggerTime = 0;
    sprintTriggerRegisteredReturn = false;
    twoJumpsRegistered = false;
    sprintTime = 0;
    m_uiInactiveTicks = 0;
    portalTime = 0.0f;
    oPortalTime = 0.0f;
    jumpRidingTicks = 0;
    jumpRidingScale = 0.0f;

    yBob = xBob = yBobO = xBobO = 0.0f;

    this->minecraft = minecraft;
    this->dimension = dimension;

    if (user != nullptr && user->name.length() > 0) {
        customTextureUrl =
            "http://s3.amazonaws.com/MinecraftSkins/" + user->name + ".png";
    }
    if (user != nullptr) {
        this->name = user->name;
        // printf("Created LocalPlayer with name %s\n", name.c_str() );
        //  check to see if this player's xuid is in the list of special players
        MOJANG_DATA* pMojangData =
            gameServices().getMojangDataForXuid(getOnlineXuid());
        if (pMojangData) {
            customTextureUrl = pMojangData->wchSkin;
        }
    }
    input = nullptr;
    m_iPad = -1;
    m_iScreenSection =
        IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;  // assume singleplayer
                                                      // default
    m_bPlayerRespawned = false;
    ullButtonsPressed = 0LL;
    ullDpad_last = ullDpad_this = ullDpad_filtered = 0;

    // 4J-PB - moved in from the minecraft structure
    // ticks=0;
    missTime = 0;
    lastClickTick[0] = 0;
    lastClickTick[1] = 0;
    isRaining = false;

    m_bIsIdle = false;
    m_iThirdPersonView = 0;

    // 4J Stu - Added for telemetry
    SetSessionTimerStart();

    // 4J - added for auto repeat in creative mode
    lastClickState = lastClick_invalid;
    lastClickTolerance = 0.0f;

    m_bHasAwardedStayinFrosty = false;
}

LocalPlayer::~LocalPlayer() {
    if (this->input != nullptr) delete input;
}

void LocalPlayer::calculateFlight(float xa, float ya, float za) {
    xa = xa * minecraft->options->flySpeed;
    ya = 0;
    za = za * minecraft->options->flySpeed;

    flyX =
        smoothFlyX.getNewDeltaValue(xa, .35f * minecraft->options->sensitivity);
    flyY =
        smoothFlyY.getNewDeltaValue(ya, .35f * minecraft->options->sensitivity);
    flyZ =
        smoothFlyZ.getNewDeltaValue(za, .35f * minecraft->options->sensitivity);
}

void LocalPlayer::serverAiStep() {
    Player::serverAiStep();

    if (abilities.flying && abilities.mayfly) {
        // snap y rotation for flying to nearest 90 degrees in world space
        float fMag = sqrtf(input->xa * input->xa + input->ya * input->ya);
        // Don't bother for tiny inputs
        if (fMag >= 0.1f) {
            // Get angle (in player rotated space) of input controls
            float yRotInput =
                atan2f(input->ya, input->xa) * (180.0f / std::numbers::pi);
            // Now get in world space
            float yRotFinal = yRotInput + yRot;
            // Snap this to nearest 90 degrees
            float yRotSnapped = floorf((yRotFinal / 45.0f) + 0.5f) * 45.0f;
            // Find out how much we had to move to do this snap
            float yRotDiff = yRotSnapped - yRotFinal;
            // Apply the same difference to the player rotated space angle
            float yRotInputAdjust = yRotInput + yRotDiff;

            // Calculate final x/y player-space movement required
            this->xxa =
                cos(yRotInputAdjust * (std::numbers::pi / 180.0f)) * fMag;
            this->yya =
                sin(yRotInputAdjust * (std::numbers::pi / 180.0f)) * fMag;
        } else {
            this->xxa = input->xa;
            this->yya = input->ya;
        }
    } else {
        this->xxa = input->xa;
        this->yya = input->ya;
    }
    this->jumping = input->jumping;

    yBobO = yBob;
    xBobO = xBob;
    xBob += (xRot - xBob) * 0.5;
    yBob += (yRot - yBob) * 0.5;

    // TODO 4J - Remove
    // if (input->jumping)
    //	mapPlayerChunk(8);
}

bool LocalPlayer::isEffectiveAi() { return true; }

void LocalPlayer::aiStep() {
    if (sprintTime > 0) {
        sprintTime--;
        if (sprintTime == 0) {
            setSprinting(false);
        }
    }
    if (sprintTriggerTime > 0) sprintTriggerTime--;
    if (minecraft->gameMode->isCutScene()) {
        x = z = 0.5;
        x = 0;
        z = 0;
        yRot = tickCount / 12.0f;
        xRot = 10;
        y = 68.5;
        return;
    }
    oPortalTime = portalTime;
    if (isInsidePortal) {
        if (!level->isClientSide) {
            if (riding != nullptr) this->ride(nullptr);
        }
        if (minecraft->screen != nullptr) minecraft->setScreen(nullptr);

        if (portalTime == 0) {
            minecraft->soundEngine->playUI(eSoundType_PORTAL_TRIGGER, 1,
                                           random->nextFloat() * 0.4f + 0.8f);
        }
        portalTime += 1 / 80.0f;
        if (portalTime >= 1) {
            portalTime = 1;
        }
        isInsidePortal = false;
    } else if (hasEffect(MobEffect::confusion) &&
               getEffect(MobEffect::confusion)->getDuration() >
                   (SharedConstants::TICKS_PER_SECOND * 3)) {
        portalTime += 1 / 150.0f;
        if (portalTime > 1) {
            portalTime = 1;
        }
    } else {
        if (portalTime > 0) portalTime -= 1 / 20.0f;
        if (portalTime < 0) portalTime = 0;
    }

    if (changingDimensionDelay > 0) changingDimensionDelay--;
    bool wasJumping = input->jumping;
    float runTreshold = 0.8f;

    bool wasRunning = input->ya >= runTreshold;
    // input->tick( std::dynamic_pointer_cast<Player>( shared_from_this() ) );
    //  4J-PB - make it a localplayer
    input->tick(this);
    if (isUsingItem() && !isRiding()) {
        input->xa *= 0.2f;
        input->ya *= 0.2f;
        sprintTriggerTime = 0;
    }
    // this.heightOffset = input.sneaking?1.30f:1.62f;	// 4J - this was already
    // commented out
    if (input->sneaking)  // 4J - removed - TODO replace
    {
        if (ySlideOffset < 0.2f) ySlideOffset = 0.2f;
    }

    checkInTile(x - bbWidth * 0.35, bb.y0 + 0.5, z + bbWidth * 0.35);
    checkInTile(x - bbWidth * 0.35, bb.y0 + 0.5, z - bbWidth * 0.35);
    checkInTile(x + bbWidth * 0.35, bb.y0 + 0.5, z - bbWidth * 0.35);
    checkInTile(x + bbWidth * 0.35, bb.y0 + 0.5, z + bbWidth * 0.35);

    bool enoughFoodToSprint =
        getFoodData()->getFoodLevel() >
        FoodConstants::MAX_FOOD * FoodConstants::FOOD_SATURATION_LOW;

    // 4J Stu - If we can fly, then we should be able to sprint without
    // requiring food. This is particularly a problem for people who save a
    // survival world with low food, then reload it in creative.
    if (abilities.mayfly || isAllowedToFly()) enoughFoodToSprint = true;

    // 4J - altered this slightly to make sure that the joypad returns to below
    // returnTreshold in between registering two movements up to runThreshold
    if (onGround && !isSprinting() && enoughFoodToSprint && !isUsingItem() &&
        !hasEffect(MobEffect::blindness)) {
        if (!wasRunning && (input->ya >= runTreshold)) {
            if (sprintTriggerTime == 0) {
                sprintTriggerTime = 7;
                sprintTriggerRegisteredReturn = false;
            } else {
                if (sprintTriggerRegisteredReturn) {
                    setSprinting(true);
                    sprintTriggerTime = 0;
                    sprintTriggerRegisteredReturn = false;
                }
            }
        } else if ((sprintTriggerTime > 0) &&
                   (input->ya == 0.0f))  // ya of 0.0f here signifies that we
                                         // have returned to the deadzone
        {
            sprintTriggerRegisteredReturn = true;
        } else if (input->sprintKey) {
            setSprinting(true);
        }
    }
    if (isSneaking()) sprintTriggerTime = 0;
    // 4J-PB - try not stopping sprint on collision
    // if (isSprinting() && (input->ya < runTreshold || horizontalCollision ||
    // !enoughFoodToSprint))
    if (isSprinting() && ((input->ya < runTreshold && !input->sprintKey) ||
                          !enoughFoodToSprint)) {
        setSprinting(false);
    }

    // 4J Stu - Fix for #52705 - Customer Encountered: Player can fly in bed
    // while being in Creative mode.
    if (!isSleeping() && (abilities.mayfly || isAllowedToFly())) {
        // 4J altered to require jump button to released after being tapped
        // twice to trigger move between flying / not flying
        if (!wasJumping && input->jumping) {
            if (jumpTriggerTime == 0) {
                jumpTriggerTime = 10;  // was 7
                twoJumpsRegistered = false;
            } else {
                twoJumpsRegistered = true;
            }
        } else if ((!input->jumping) && (jumpTriggerTime > 0) &&
                   twoJumpsRegistered) {
#if !defined(_CONTENT_PACKAGE)
            printf("flying was %s\n", abilities.flying ? "on" : "off");
#endif
            abilities.flying = !abilities.flying;
#if !defined(_CONTENT_PACKAGE)
            printf("flying is %s\n", abilities.flying ? "on" : "off");
#endif
            jumpTriggerTime = 0;
            twoJumpsRegistered = false;
            if (abilities.flying)
                input->sneaking =
                    false;  // 4J added - would we ever intentially want to go
                            // into flying mode whilst sneaking?
        }
    } else if (abilities.flying) {
#if defined(_DEBUG_MENUS_ENABLED)
        if (!abilities.debugflying)
#endif
        {
            abilities.flying = false;
        }
    }

    if (abilities.flying) {
        //            yd = 0;
        // 4J - note that the 0.42 added for going down is to make it match with
        // what happens when you jump - jumping itself adds 0.42 to yd in
        // Mob::jumpFromGround
        if (ullButtonsPressed & (1LL << MINECRAFT_ACTION_SNEAK_TOGGLE))
            yd -=
                (0.15 + 0.42);  // 4J - for flying mode,
                                // MINECRAFT_ACTION_SNEAK_TOGGLE isn't a toggle
                                // but just indicates that this button is down
        if (input->jumping) {
            noJumpDelay = 0;
            yd += 0.15;
        }

        // snap y rotation to nearest 90 degree axis aligned value
        float yRotSnapped = floorf((yRot / 90.0f) + 0.5f) * 90.0f;

        if (PlatformInput.GetJoypadMapVal(m_iPad) == 0) {
            if (ullDpad_filtered & (1LL << MINECRAFT_ACTION_DPAD_RIGHT)) {
                xd = -0.15 * cos(yRotSnapped * std::numbers::pi / 180);
                zd = -0.15 * sin(yRotSnapped * std::numbers::pi / 180);
            } else if (ullDpad_filtered & (1LL << MINECRAFT_ACTION_DPAD_LEFT)) {
                xd = 0.15 * cos(yRotSnapped * std::numbers::pi / 180);
                zd = 0.15 * sin(yRotSnapped * std::numbers::pi / 180);
            }
        }
    }

    if (isRidingJumpable()) {
        if (jumpRidingTicks < 0) {
            jumpRidingTicks++;
            if (jumpRidingTicks == 0) {
                // reset scale (for gui)
                jumpRidingScale = 0;
            }
        }
        if (wasJumping && !input->jumping) {
            // jump release
            jumpRidingTicks = -10;
            sendRidingJump();
        } else if (!wasJumping && input->jumping) {
            // jump press
            jumpRidingTicks = 0;
            jumpRidingScale = 0;
        } else if (wasJumping) {
            // calc jump scale
            jumpRidingTicks++;
            if (jumpRidingTicks < 10) {
                jumpRidingScale = (float)jumpRidingTicks * .1f;
            } else {
                jumpRidingScale =
                    .8f + (2.f / ((float)(jumpRidingTicks - 9))) * .1f;
            }
        }
    } else {
        jumpRidingScale = 0;
    }

    Player::aiStep();

    // 4J-PB - If we're in Creative Mode, allow flying on ground
    if (!abilities.mayfly && !isAllowedToFly()) {
        if (onGround && abilities.flying) {
#if defined(_DEBUG_MENUS_ENABLED)
            if (!abilities.debugflying)
#endif
            {
                abilities.flying = false;
            }
        }
    }

    if (abilities.flying)  // minecraft->options->isFlying )
    {
        Vec3 viewVector = getViewVector(1.0f);

        // 4J-PB - To let the player build easily while flying, we need to
        // change this

#if defined(_DEBUG_MENUS_ENABLED)
        if (abilities.debugflying) {
            flyX = (float)viewVector.x * input->ya;
            flyY = (float)viewVector.y * input->ya;
            flyZ = (float)viewVector.z * input->ya;
        } else
#endif
        {
            if (isSprinting()) {
                // Accelrate up to full speed if we are sprinting, moving in the
                // direction of the view vector
                flyX = (float)viewVector.x * input->ya;
                flyY = (float)viewVector.y * input->ya;
                flyZ = (float)viewVector.z * input->ya;

                float scale = ((float)(SPRINT_DURATION - sprintTime)) / 10.0f;
                scale = scale * scale;
                if (scale > 1.0f) scale = 1.0f;
                flyX *= scale;
                flyY *= scale;
                flyZ *= scale;
            } else {
                flyX = 0.0f;
                flyY = 0.0f;
                flyZ = 0.0f;
                if (ullDpad_filtered & (1LL << MINECRAFT_ACTION_DPAD_UP)) {
                    flyY = 0.1f;
                }
                if (ullDpad_filtered & (1LL << MINECRAFT_ACTION_DPAD_DOWN)) {
                    flyY = -0.1f;
                }
            }
        }

        Player::move(flyX, flyY, flyZ);

        fallDistance = 0.0f;
        yd = 0.0f;
        onGround = true;
    }

    // Check if the player is idle and the rich presence needs updated
    if (!m_bIsIdle && PlatformInput.GetIdleSeconds(m_iPad) > PLAYER_IDLE_TIME) {
        PlatformProfile.SetCurrentGameActivity(m_iPad, CONTEXT_PRESENCE_IDLE,
                                               false);
        m_bIsIdle = true;
    } else if (m_bIsIdle &&
               PlatformInput.GetIdleSeconds(m_iPad) < PLAYER_IDLE_TIME) {
        // Are we offline or online, and how many players are there
        if (NetworkService.GetPlayerCount() > 1) {
            // only do it for this player here - each player will run this code
            if (NetworkService.IsLocalGame()) {
                PlatformProfile.SetCurrentGameActivity(
                    m_iPad, CONTEXT_PRESENCE_MULTIPLAYEROFFLINE, false);
            } else {
                PlatformProfile.SetCurrentGameActivity(
                    m_iPad, CONTEXT_PRESENCE_MULTIPLAYER, false);
            }
        } else {
            if (NetworkService.IsLocalGame()) {
                PlatformProfile.SetCurrentGameActivity(
                    m_iPad, CONTEXT_PRESENCE_MULTIPLAYER_1POFFLINE, false);
            } else {
                PlatformProfile.SetCurrentGameActivity(
                    m_iPad, CONTEXT_PRESENCE_MULTIPLAYER_1P, false);
            }
        }
        updateRichPresence();
        m_bIsIdle = false;
    }
}

void LocalPlayer::changeDimension(int i) {
    if (!level->isClientSide) {
        if (dimension == 1 && i == 1) {
            awardStat(GenericStats::winGame(), GenericStats::param_noArgs());
            // minecraft.setScreen(new WinScreen());
#if !defined(_CONTENT_PACKAGE)
            Log::info(
                "LocalPlayer::changeDimension from 1 to 1 but WinScreen has "
                "not been implemented.\n");
            assert(0);
#endif
        } else {
            awardStat(GenericStats::theEnd(), GenericStats::param_theEnd());

            minecraft->soundEngine->playUI(eSoundType_PORTAL_TRAVEL, 1,
                                           random->nextFloat() * 0.4f + 0.8f);
        }
    }
}

float LocalPlayer::getFieldOfViewModifier() {
    float targetFov = 1.0f;

    // modify for movement
    if (abilities.flying) targetFov *= 1.1f;

    AttributeInstance* speed =
        getAttribute(SharedMonsterAttributes::MOVEMENT_SPEED);
    targetFov *= (speed->getValue() / abilities.getWalkingSpeed() + 1) / 2;

    // modify for bow =)
    if (isUsingItem() && getUseItem()->id == Item::bow->id) {
        int ticksHeld = getTicksUsingItem();
        float scale = (float)ticksHeld / BowItem::MAX_DRAW_DURATION;
        if (scale > 1) {
            scale = 1;
        } else {
            scale *= scale;
        }
        targetFov *= 1.0f - scale * .15f;
    }

    return targetFov;
}

void LocalPlayer::addAdditonalSaveData(CompoundTag* entityTag) {
    Player::addAdditonalSaveData(entityTag);
    // entityTag->putInt("Score", score);
}

void LocalPlayer::readAdditionalSaveData(CompoundTag* entityTag) {
    Player::readAdditionalSaveData(entityTag);
    // score = entityTag->getInt("Score");
}

void LocalPlayer::closeContainer() {
    Player::closeContainer();
    minecraft->setScreen(nullptr);

    // 4J - Close any xui here
    // Fix for #9164 - CRASH: MP: Title crashes upon opening a chest and having
    // another user destroy it.
    ui.PlayUISFX(eSFX_Back);
    ui.CloseUIScenes(m_iPad);
}

void LocalPlayer::openTextEdit(std::shared_ptr<TileEntity> tileEntity) {
#if defined(ENABLE_JAVA_GUIS)
    if (tileEntity->GetType() == eTYPE_SIGNTILEENTITY) {
        minecraft->setScreen(new TextEditScreen(
            std::dynamic_pointer_cast<SignTileEntity>(tileEntity)));
        bool success = true;
    }
#else
    bool success;

    if (tileEntity->GetType() == eTYPE_SIGNTILEENTITY) {
        success = gameServices().menus().openSign(
            GetXboxPad(),
            std::dynamic_pointer_cast<SignTileEntity>(tileEntity));
    } else if (tileEntity->GetType() == eTYPE_COMMANDBLOCKTILEENTITY) {
        success = gameServices().menus().openCommandBlock(
            GetXboxPad(),
            std::dynamic_pointer_cast<CommandBlockEntity>(tileEntity));
    }

    if (success) ui.PlayUISFX(eSFX_Press);
#endif
}

bool LocalPlayer::openContainer(std::shared_ptr<Container> container) {
#if defined(ENABLE_JAVA_GUIS)
    minecraft->setScreen(new ContainerScreen(inventory, container));
    bool success = true;
#else
    bool success = gameServices().menus().openContainer(GetXboxPad(), inventory,
                                                        container);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    // minecraft->setScreen(new ContainerScreen(inventory, container));
    return success;
}

bool LocalPlayer::openHopper(std::shared_ptr<HopperTileEntity> container) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new HopperScreen(inventory, container));
    bool success = true;
#else
    bool success =
        gameServices().menus().openHopper(GetXboxPad(), inventory, container);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openHopper(std::shared_ptr<MinecartHopper> container) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new HopperScreen(inventory, container));
    bool success = true;
#else
    bool success = gameServices().menus().openHopperMinecart(
        GetXboxPad(), inventory, container);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openHorseInventory(std::shared_ptr<EntityHorse> horse,
                                     std::shared_ptr<Container> container) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new HorseInventoryScreen(inventory, container, horse));
    bool success = true;
#else
    bool success = gameServices().menus().openHorse(GetXboxPad(), inventory,
                                                    container, horse);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::startCrafting(int x, int y, int z) {
#if defined(ENABLE_JAVA_GUIS)
    minecraft->setScreen(new CraftingScreen(inventory, level, x, y, z));
    bool success = true;
#else
    bool success = gameServices().menus().openCrafting3x3(
        GetXboxPad(),
        std::dynamic_pointer_cast<LocalPlayer>(shared_from_this()), x, y, z);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    // app.LoadXuiCraftMenu(0,inventory, level, x, y, z);
    // minecraft->setScreen(new CraftingScreen(inventory, level, x, y, z));
    return success;
}

bool LocalPlayer::openFireworks(int x, int y, int z) {
    bool success = gameServices().menus().openFireworks(
        GetXboxPad(),
        std::dynamic_pointer_cast<LocalPlayer>(shared_from_this()), x, y, z);
    if (success) ui.PlayUISFX(eSFX_Press);
    return success;
}

bool LocalPlayer::startEnchanting(int x, int y, int z,
                                  const std::string& name) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new EnchantmentScreen(inventory, level, x, y, z));
    bool success = true;
#else
    bool success = gameServices().menus().openEnchanting(
        GetXboxPad(), inventory, x, y, z, level, name);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::startRepairing(int x, int y, int z) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new RepairScreen(inventory, level, x, y, z));
    bool success = true;
#else
    bool success = gameServices().menus().openRepairing(GetXboxPad(), inventory,
                                                        level, x, y, z);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openFurnace(std::shared_ptr<FurnaceTileEntity> furnace) {
#if defined(ENABLE_JAVA_GUIS)
    minecraft->setScreen(new FurnaceScreen(inventory, furnace));
    bool success = true;
#else
    bool success =
        gameServices().menus().openFurnace(GetXboxPad(), inventory, furnace);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openBrewingStand(
    std::shared_ptr<BrewingStandTileEntity> brewingStand) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new BrewingStandScreen(inventory, brewingStand));
    bool success = true;
#else
    bool success = gameServices().menus().openBrewingStand(
        GetXboxPad(), inventory, brewingStand);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openBeacon(std::shared_ptr<BeaconTileEntity> beacon) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new BeaconScreen(inventory, beacon));
    bool success = true;
#else
    bool success =
        gameServices().menus().openBeacon(GetXboxPad(), inventory, beacon);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openTrap(std::shared_ptr<DispenserTileEntity> trap) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new TrapScreen(inventory, trap));
    bool success = true;
#else
    bool success =
        gameServices().menus().openTrap(GetXboxPad(), inventory, trap);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

bool LocalPlayer::openTrading(std::shared_ptr<Merchant> traderTarget,
                              const std::string& name) {
#ifdef ENABLE_JAVA_GUIS
    minecraft->setScreen(new MerchantScreen(inventory, traderTarget, level));
    bool success = true;
#else
    bool success = gameServices().menus().openTrading(
        GetXboxPad(), inventory, traderTarget, level, name);
    if (success) ui.PlayUISFX(eSFX_Press);
#endif
    return success;
}

void LocalPlayer::crit(std::shared_ptr<Entity> e) {
    std::shared_ptr<CritParticle> critParticle = std::shared_ptr<CritParticle>(
        new CritParticle((Level*)minecraft->level, e));
    critParticle->CritParticlePostConstructor();
    minecraft->particleEngine->add(critParticle);
}

void LocalPlayer::magicCrit(std::shared_ptr<Entity> e) {
    std::shared_ptr<CritParticle> critParticle = std::shared_ptr<CritParticle>(
        new CritParticle((Level*)minecraft->level, e, eParticleType_magicCrit));
    critParticle->CritParticlePostConstructor();
    minecraft->particleEngine->add(critParticle);
}

void LocalPlayer::take(std::shared_ptr<Entity> e, int orgCount) {
    minecraft->particleEngine->add(std::make_shared<TakeAnimationParticle>(
        (Level*)minecraft->level, e, shared_from_this(), -0.5f));
}

void LocalPlayer::chat(const std::string& message) {}

bool LocalPlayer::isSneaking() { return input->sneaking && !m_isSleeping; }

void LocalPlayer::hurtTo(float newHealth, uint8_t damageSource) {
    float dmg = getHealth() - newHealth;
    if (dmg <= 0) {
        setHealth(newHealth);
        if (dmg < 0) {
            invulnerableTime = invulnerableDuration / 2;
        }
    } else {
        lastHurt = dmg;
        setHealth(getHealth());
        invulnerableTime = invulnerableDuration;
        actuallyHurt(DamageSource::genericSource, dmg);
        hurtTime = hurtDuration = 10;
    }

    if (this->getHealth() <= 0) {
        int deathTime =
            (int)(level->getGameTime() % Level::TICKS_PER_DAY) / 1000;
        int carriedId = inventory->getSelected() == nullptr
                            ? 0
                            : inventory->getSelected()->id;

        // if there are any xuiscenes up for this player, close them
        if (ui.GetMenuDisplayed(GetXboxPad())) {
            ui.CloseUIScenes(GetXboxPad());
        }
    }
}

void LocalPlayer::respawn() {
    // Select the right payer to respawn
    minecraft->respawnPlayer(GetXboxPad(), 0, 0);
}

void LocalPlayer::animateRespawn() {
    //        Player.animateRespawn(this, level);
}

void LocalPlayer::displayClientMessage(int messageId) {
    minecraft->gui->displayClientMessage(messageId, GetXboxPad());
}

void LocalPlayer::awardStat(Stat* stat, const std::vector<uint8_t>& param) {
    int count = CommonStats::readParam(param);

    if (!gameServices().canRecordStatsAndAchievements()) return;
    if (stat == nullptr) return;

    if (stat->isAchievement()) {
        Achievement* ach = (Achievement*)stat;
        // 4J-PB - changed to attempt to award everytime - the award may need a
        // storage device, so needs a primary player, and the player may not
        // have been a primary player when they first 'got' the award so let the
        // award manager figure it out
        if (!minecraft->stats[m_iPad]->hasTaken(ach)) {
            // 4J-PB - Don't display the java popup
#if defined(ENABLE_JAVA_GUIS)
            minecraft->achievementPopup->popup(ach);
#endif

            // 4J Stu - Added this function in the libraries as some
            // achievements don't get awarded to all players e.g. Splitscreen
            // players cannot get theme/avatar/gamerpic and Trial players cannot
            // get any This causes some extreme flooding of some awards
            if (PlatformProfile.CanBeAwarded(m_iPad, ach->getAchievementID())) {
                // 4J Stu - Some awards cause a menu to popup. This can be bad,
                // especially if you are surrounded by mobs! We cannot pause the
                // game unless in offline single player, but lets at least do it
                // then
                if (NetworkService.IsLocalGame() &&
                    NetworkService.GetPlayerCount() == 1 &&
                    PlatformProfile.GetAwardType(ach->getAchievementID()) !=
                        EAwardType::Achievement) {
                    ui.CloseUIScenes(m_iPad);
                    ui.NavigateToScene(m_iPad, eUIScene_PauseMenu);
                }
            }

            // 4J-JEV: To stop spamming trophies.
            unsigned long long achBit = ((unsigned long long)1)
                                        << ach->getAchievementID();
            if (!(achBit & m_awardedThisSession)) {
                PlatformProfile.Award(m_iPad, ach->getAchievementID());
                m_awardedThisSession |= achBit;
            }
        }
        minecraft->stats[m_iPad]->award(stat, level->difficulty, count);
    } else {
        // 4J : WESTY : Added for new achievements.
        StatsCounter* pStats = minecraft->stats[m_iPad];
        pStats->award(stat, level->difficulty, count);

        // 4J-JEV: Check achievements for unlocks.

        // LEADER OF THE PACK
        if (stat == GenericStats::tamedEntity(eTYPE_WOLF)) {
            // Check to see if we have befriended 5 wolves! Is this really the
            // best place to do this??!!
            if (pStats->getTotalValue(GenericStats::tamedEntity(eTYPE_WOLF)) >=
                5) {
                awardStat(GenericStats::leaderOfThePack(),
                          GenericStats::param_noArgs());
            }
        }

        // MOAR TOOLS
        {
            Stat* toolStats[4][5];
            toolStats[0][0] = GenericStats::itemsCrafted(Item::shovel_wood->id);
            toolStats[0][1] =
                GenericStats::itemsCrafted(Item::shovel_stone->id);
            toolStats[0][2] = GenericStats::itemsCrafted(Item::shovel_iron->id);
            toolStats[0][3] =
                GenericStats::itemsCrafted(Item::shovel_diamond->id);
            toolStats[0][4] = GenericStats::itemsCrafted(Item::shovel_gold->id);
            toolStats[1][0] =
                GenericStats::itemsCrafted(Item::pickAxe_wood->id);
            toolStats[1][1] =
                GenericStats::itemsCrafted(Item::pickAxe_stone->id);
            toolStats[1][2] =
                GenericStats::itemsCrafted(Item::pickAxe_iron->id);
            toolStats[1][3] =
                GenericStats::itemsCrafted(Item::pickAxe_diamond->id);
            toolStats[1][4] =
                GenericStats::itemsCrafted(Item::pickAxe_gold->id);
            toolStats[2][0] =
                GenericStats::itemsCrafted(Item::hatchet_wood->id);
            toolStats[2][1] =
                GenericStats::itemsCrafted(Item::hatchet_stone->id);
            toolStats[2][2] =
                GenericStats::itemsCrafted(Item::hatchet_iron->id);
            toolStats[2][3] =
                GenericStats::itemsCrafted(Item::hatchet_diamond->id);
            toolStats[2][4] =
                GenericStats::itemsCrafted(Item::hatchet_gold->id);
            toolStats[3][0] = GenericStats::itemsCrafted(Item::hoe_wood->id);
            toolStats[3][1] = GenericStats::itemsCrafted(Item::hoe_stone->id);
            toolStats[3][2] = GenericStats::itemsCrafted(Item::hoe_iron->id);
            toolStats[3][3] = GenericStats::itemsCrafted(Item::hoe_diamond->id);
            toolStats[3][4] = GenericStats::itemsCrafted(Item::hoe_gold->id);

            bool justCraftedTool = false;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 5; j++) {
                    if (stat == toolStats[i][j]) {
                        justCraftedTool = true;
                        break;
                    }
                }
            }

            if (justCraftedTool) {
                bool awardNow = true;
                for (int i = 0; i < 4; i++) {
                    bool craftedThisTool = false;
                    for (int j = 0; j < 5; j++) {
                        if (pStats->getTotalValue(toolStats[i][j]) > 0)
                            craftedThisTool = true;
                    }

                    if (!craftedThisTool) {
                        awardNow = false;
                        break;
                    }
                }

                if (awardNow) {
                    awardStat(GenericStats::MOARTools(),
                              GenericStats::param_noArgs());
                }
            }
        }

#if defined(_EXTENDED_ACHIEVEMENTS)

        // AWARD : Porkchop, cook and eat a porkchop.
        {
            Stat *cookPorkchop, *eatPorkchop;
            cookPorkchop = GenericStats::itemsCrafted(Item::porkChop_cooked_Id);
            eatPorkchop = GenericStats::itemsUsed(Item::porkChop_cooked_Id);

            if (stat == cookPorkchop || stat == eatPorkchop) {
                int numCookPorkchop, numEatPorkchop;
                numCookPorkchop = pStats->getTotalValue(cookPorkchop);
                numEatPorkchop = pStats->getTotalValue(eatPorkchop);

                Log::info(
                    "[AwardStat] Check unlock 'Porkchop': "
                    "pork_cooked=%i, pork_eaten=%i.\n",
                    numCookPorkchop, numEatPorkchop);

                if ((0 < numCookPorkchop) && (0 < numEatPorkchop)) {
                    awardStat(GenericStats::porkChop(),
                              GenericStats::param_porkChop());
                }
            }
        }

        // AWARD : Passing the Time, play for 100 minecraft days.
        {
            Stat* timePlayed = GenericStats::timePlayed();

            if (stat == timePlayed) {
                int iPlayedTicks, iRequiredTicks;
                iPlayedTicks = pStats->getTotalValue(timePlayed);
                iRequiredTicks = Level::TICKS_PER_DAY * 100;

                /* Log::info(
                        "[AwardStat] Check unlock 'Passing the Time': "
                        "total_ticks=%i, req=%i.\n",
                        iPlayedTicks, iRequiredTicks
                        ); */

                if (iPlayedTicks >= iRequiredTicks) {
                    awardStat(GenericStats::passingTheTime(),
                              GenericStats::param_passingTheTime());
                }
            }
        }

        // AWARD : The Haggler, Acquire 30 emeralds.
        {
            Stat *emeraldMined, *emeraldBought;
            emeraldMined = GenericStats::blocksMined(Tile::emeraldOre_Id);
            emeraldBought = GenericStats::itemsBought(Item::emerald_Id);

            if (stat == emeraldMined || stat == emeraldBought) {
                int numEmeraldMined, numEmeraldBought, totalSum;
                numEmeraldMined = pStats->getTotalValue(emeraldMined);
                numEmeraldBought = pStats->getTotalValue(emeraldBought);
                totalSum = numEmeraldMined + numEmeraldBought;

                Log::info(
                    "[AwardStat] Check unlock 'The Haggler': "
                    "emerald_mined=%i, emerald_bought=%i, sum=%i.\n",
                    numEmeraldMined, numEmeraldBought, totalSum);

                if (totalSum >= 30)
                    awardStat(GenericStats::theHaggler(),
                              GenericStats::param_theHaggler());
            }
        }

        // AWARD : Pot Planter, craft and place a flowerpot.
        {
            Stat *craftFlowerpot, *placeFlowerpot;
            craftFlowerpot = GenericStats::itemsCrafted(Item::flowerPot_Id);
            placeFlowerpot = GenericStats::blocksPlaced(Tile::flowerPot_Id);

            if (stat == craftFlowerpot || stat == placeFlowerpot) {
                if ((pStats->getTotalValue(craftFlowerpot) > 0) &&
                    (pStats->getTotalValue(placeFlowerpot) > 0)) {
                    awardStat(GenericStats::potPlanter(),
                              GenericStats::param_potPlanter());
                }
            }
        }

        // AWARD : It's a Sign, craft and place a sign.
        {
            Stat *craftSign, *placeWallsign, *placeSignpost;
            craftSign = GenericStats::itemsCrafted(Item::sign_Id);
            placeWallsign = GenericStats::blocksPlaced(Tile::wallSign_Id);
            placeSignpost = GenericStats::blocksPlaced(Tile::sign_Id);

            if (stat == craftSign || stat == placeWallsign ||
                stat == placeSignpost) {
                int numCraftedSigns, numPlacedWallSign, numPlacedSignpost;
                numCraftedSigns = pStats->getTotalValue(craftSign);
                numPlacedWallSign = pStats->getTotalValue(placeWallsign);
                numPlacedSignpost = pStats->getTotalValue(placeSignpost);

                Log::info(
                    "[AwardStat] Check unlock 'It's a Sign': "
                    "crafted=%i, placedWallSigns=%i, placedSignposts=%i.\n",
                    numCraftedSigns, numPlacedWallSign, numPlacedSignpost);

                if ((numCraftedSigns > 0) &&
                    ((numPlacedWallSign + numPlacedSignpost) > 0)) {
                    awardStat(GenericStats::itsASign(),
                              GenericStats::param_itsASign());
                }
            }
        }

        // AWARD : Rainbow Collection, collect all different colours of wool.
        {
            bool justPickedupWool = false;

            for (int i = 0; i < 16; i++)
                if (stat == GenericStats::itemsCollected(Tile::wool_Id, i))
                    justPickedupWool = true;

            if (justPickedupWool) {
                unsigned int woolCount = 0;

                for (unsigned int i = 0; i < 16; i++) {
                    if (pStats->getTotalValue(
                            GenericStats::itemsCollected(Tile::wool_Id, i)) > 0)
                        woolCount++;
                }

                if (woolCount >= 16)
                    awardStat(GenericStats::rainbowCollection(),
                              GenericStats::param_rainbowCollection());
            }
        }

        // AWARD : Adventuring Time, visit at least 17 biomes
        {
            bool justEnteredBiome = false;

            for (int i = 0; i < 23; i++)
                if (stat == GenericStats::enteredBiome(i))
                    justEnteredBiome = true;

            if (justEnteredBiome) {
                unsigned int biomeCount = 0;

                for (unsigned int i = 0; i < 23; i++) {
                    if (pStats->getTotalValue(GenericStats::enteredBiome(i)) >
                        0)
                        biomeCount++;
                }

                if (biomeCount >= 17)
                    awardStat(GenericStats::adventuringTime(),
                              GenericStats::param_adventuringTime());
            }
        }
#endif
    }
}

bool LocalPlayer::isSolidBlock(int x, int y, int z) {
    return level->isSolidBlockingTile(x, y, z);
}

bool LocalPlayer::checkInTile(double x, double y, double z) {
    int xTile = std::floor(x);
    int yTile = std::floor(y);
    int zTile = std::floor(z);

    double xd = x - xTile;
    double zd = z - zTile;

    if (isSolidBlock(xTile, yTile, zTile) ||
        isSolidBlock(xTile, yTile + 1, zTile)) {
        bool west = !isSolidBlock(xTile - 1, yTile, zTile) &&
                    !isSolidBlock(xTile - 1, yTile + 1, zTile);
        bool east = !isSolidBlock(xTile + 1, yTile, zTile) &&
                    !isSolidBlock(xTile + 1, yTile + 1, zTile);
        bool north = !isSolidBlock(xTile, yTile, zTile - 1) &&
                     !isSolidBlock(xTile, yTile + 1, zTile - 1);
        bool south = !isSolidBlock(xTile, yTile, zTile + 1) &&
                     !isSolidBlock(xTile, yTile + 1, zTile + 1);

        int dir = -1;
        double closest = 9999;
        if (west && xd < closest) {
            closest = xd;
            dir = 0;
        }
        if (east && 1 - xd < closest) {
            closest = 1 - xd;
            dir = 1;
        }
        if (north && zd < closest) {
            closest = zd;
            dir = 4;
        }
        if (south && 1 - zd < closest) {
            closest = 1 - zd;
            dir = 5;
        }

        float speed = 0.1f;
        if (dir == 0) this->xd = -speed;
        if (dir == 1) this->xd = +speed;
        if (dir == 4) this->zd = -speed;
        if (dir == 5) this->zd = +speed;
    }

    return false;
}

void LocalPlayer::setSprinting(bool value) {
    Player::setSprinting(value);
    if (value == false)
        sprintTime = 0;
    else
        sprintTime = SPRINT_DURATION;
}

void LocalPlayer::setExperienceValues(float experienceProgress, int totalExp,
                                      int experienceLevel) {
    this->experienceProgress = experienceProgress;
    this->totalExperience = totalExp;
    this->experienceLevel = experienceLevel;
}

// 4J: removed
// void LocalPlayer::sendMessage(ChatMessageComponent *message)
//{
//	minecraft->gui->getChat()->addMessage(message.toString(true));
//}

Pos LocalPlayer::getCommandSenderWorldPosition() {
    return new Pos(floor(x + .5), floor(y + .5), floor(z + .5));
}

std::shared_ptr<ItemInstance> LocalPlayer::getCarriedItem() {
    return inventory->getSelected();
}

void LocalPlayer::playSound(int soundId, float volume, float pitch) {
    level->playLocalSound(x, y - heightOffset, z, soundId, volume, pitch,
                          false);
}

bool LocalPlayer::isRidingJumpable() {
    return riding != nullptr && riding->GetType() == eTYPE_HORSE;
}

float LocalPlayer::getJumpRidingScale() { return jumpRidingScale; }

void LocalPlayer::sendRidingJump() {}

bool LocalPlayer::hasPermission(EGameCommand command) {
    return level->getLevelData()->getAllowCommands();
}

void LocalPlayer::onCrafted(std::shared_ptr<ItemInstance> item) {
    if (minecraft->localgameModes[m_iPad] != nullptr) {
        TutorialMode* gameMode =
            (TutorialMode*)minecraft->localgameModes[m_iPad];
        gameMode->getTutorial()->onCrafted(item);
    }
}

void LocalPlayer::setAndBroadcastCustomSkin(std::uint32_t skinId) {
    setCustomSkin(skinId);
}

void LocalPlayer::setAndBroadcastCustomCape(std::uint32_t capeId) {
    setCustomCape(capeId);
}

// 4J TODO - Remove
#include "minecraft/world/level/chunk/LevelChunk.h"

class ModelPart;

void LocalPlayer::mapPlayerChunk(const unsigned int flagTileType) {
    int cx = this->xChunk;
    int cz = this->zChunk;

    int pZ = ((int)floor(this->z)) % 16;
    int pX = ((int)floor(this->x)) % 16;

    std::cout << "player in chunk (" << cx << "," << cz << ") at (" << this->x
              << "," << this->y << "," << this->z << ")\n";

    for (int v = -1; v < 2; v++)
        for (unsigned int z = 0; z < 16; z++) {
            for (int u = -1; u < 2; u++)
                for (unsigned int x = 0; x < 16; x++) {
                    LevelChunk* cc = level->getChunk(cx + u, cz + v);
                    if (x == pX && z == pZ && u == 0 && v == 0)
                        std::cout << "O";
                    else
                        for (unsigned int y = 127; y > 0; y--) {
                            int t = cc->getTile(x, y, z);
                            if (flagTileType != 0 && t == flagTileType) {
                                std::cout << "@";
                                break;
                            } else if (t != 0 && t < 10) {
                                std::cout << t;
                                break;
                            } else if (t > 0) {
                                std::cout << "#";
                                break;
                            }
                        }
                }
            std::cout << "\n";
        }

    std::cout << "\n";
}

void LocalPlayer::handleMouseDown(int button, bool down) {
    // 4J Stu - We should not accept any input while asleep, except the above to
    // wake up
    if (isSleeping() && level != nullptr && level->isClientSide) {
        return;
    }
    if (!down) missTime = 0;
    if (button == 0 && missTime > 0) return;

    if (down && minecraft->hitResult != nullptr &&
        minecraft->hitResult->type == HitResult::TILE && button == 0) {
        int x = minecraft->hitResult->x;
        int y = minecraft->hitResult->y;
        int z = minecraft->hitResult->z;

        // 4J - addition to stop layer mining out of the top or bottom of the
        // world 4J Stu - Allow this for The End
        if (((y == 0) || ((y == 127) && level->dimension->hasCeiling)) &&
            level->dimension->id != 1)
            return;

        minecraft->gameMode->continueDestroyBlock(x, y, z,
                                                  minecraft->hitResult->f);

        if (mayDestroyBlockAt(x, y, z)) {
            minecraft->particleEngine->crack(x, y, z, minecraft->hitResult->f);
            swing();
        }
    } else {
        minecraft->gameMode->stopDestroyBlock();
    }
}

bool LocalPlayer::creativeModeHandleMouseClick(int button, bool buttonPressed) {
    if (buttonPressed) {
        if (lastClickState == lastClick_oldRepeat) {
            return false;
        }

        // Are we in an auto-repeat situation? - If so only tell the game that
        // we've clicked if we move more than a unit away from our last click
        // position in any axis
        if (lastClickState != lastClick_invalid) {
            // If we're in disabled mode already (set when sprinting) then don't
            // do anything - if we're sprinting, we don't auto-repeat at all.
            // With auto repeat on, we can quickly place fires causing
            // photosensitivity issues due to rapid flashing
            if (lastClickState == lastClick_disabled) return false;
            // If we've started sprinting, go into this mode & also don't do
            // anything Ignore repeate when sleeping
            if (isSprinting()) {
                lastClickState = lastClick_disabled;
                return false;
            }

            // Get distance from last click point in each axis
            float dX = (float)x - lastClickX;
            float dY = (float)y - lastClickY;
            float dZ = (float)z - lastClickZ;
            bool newClick = false;

            float ddx = dX - lastClickdX;
            float ddy = dY - lastClickdY;
            float ddz = dZ - lastClickdZ;

            if (lastClickState == lastClick_moving) {
                float deltaChange = sqrtf(ddx * ddx + ddy * ddy + ddz * ddz);
                if (deltaChange < 0.01f) {
                    lastClickState = lastClick_stopped;
                    lastClickTolerance = 0.0f;
                }
            } else if (lastClickState == lastClick_stopped) {
                float deltaChange = sqrtf(ddx * ddx + ddy * ddy + ddz * ddz);
                if (deltaChange >= 0.01f) {
                    lastClickState = lastClick_moving;
                    lastClickTolerance = 0.0f;
                } else {
                    lastClickTolerance += 0.1f;
                    if (lastClickTolerance > 0.7f) {
                        lastClickTolerance = 0.0f;
                        lastClickState = lastClick_init;
                    }
                }
            }

            lastClickdX = dX;
            lastClickdY = dY;
            lastClickdZ = dZ;

            // If we have moved more than one unit in any one axis, then
            // register a new click The new click position is normalised at one
            // unit in the direction of movement, so that we don't gradually
            // drift away if we detect the movement a fraction over the unit
            // distance each time

            if (fabsf(dX) >= 1.0f) {
                dX = (dX < 0.0f) ? ceilf(dX) : floorf(dX);
                newClick = true;
            } else if (fabsf(dY) >= 1.0f) {
                dY = (dY < 0.0f) ? ceilf(dY) : floorf(dY);
                newClick = true;
            } else if (fabsf(dZ) >= 1.0f) {
                dZ = (dZ < 0.0f) ? ceilf(dZ) : floorf(dZ);
                newClick = true;
            }

            if ((!newClick) && (lastClickTolerance > 0.0f)) {
                float fTarget = 1.0f - lastClickTolerance;

                if (fabsf(dX) >= fTarget) newClick = true;
                if (fabsf(dY) >= fTarget) newClick = true;
                if (fabsf(dZ) >= fTarget) newClick = true;
            }

            if (newClick) {
                lastClickX += dX;
                lastClickY += dY;
                lastClickZ += dZ;

                // Get a more accurate pick from the position where the new
                // click should ideally have come from, rather than where we
                // happen to be now (ie a rounded number of units from the last
                // Click position)
                double oldX = x;
                double oldY = y;
                double oldZ = z;
                x = lastClickX;
                y = lastClickY;
                z = lastClickZ;

                minecraft->gameRenderer->pick(1);

                x = oldX;
                y = oldY;
                z = oldZ;

                handleMouseClick(button);

                if (lastClickState == lastClick_stopped) {
                    lastClickState = lastClick_init;
                    lastClickTolerance = 0.0f;
                } else {
                    lastClickState = lastClick_moving;
                    lastClickTolerance = 0.0f;
                }
            }
        } else {
            // First click - just record position & handle
            lastClickX = (float)x;
            lastClickY = (float)y;
            lastClickZ = (float)z;
            // If we actually placed an item, then move into the init state as
            // we are going to be doing the special creative mode auto repeat
            bool itemPlaced = handleMouseClick(button);
            // If we're sprinting or riding, don't auto-repeat at all. With auto
            // repeat on, we can quickly place fires causing photosensitivity
            // issues due to rapid flashing Also ignore repeats when the player
            // is sleeping
            if (isSprinting() || isRiding() || isSleeping()) {
                lastClickState = lastClick_disabled;
            } else {
                if (itemPlaced) {
                    lastClickState = lastClick_init;
                    lastClickTolerance = 0.0f;
                } else {
                    // Didn't place an item - might actually be activating a
                    // switch or door or something - just do a standard auto
                    // repeat in this case
                    lastClickState = lastClick_oldRepeat;
                }
            }
            return true;
        }
    } else {
        lastClickState = lastClick_invalid;
    }
    return false;
}

bool LocalPlayer::handleMouseClick(int button) {
    bool returnItemPlaced = false;

    if (button == 0 && missTime > 0) return false;
    if (button == 0) {
        // Log::info("handleMouseClick - Player %d is
        // swinging\n",GetXboxPad());
        swing();
    }

    bool mayUse = true;

    // 4J-PB - Adding a special case in here for sleeping in a bed in a
    // multiplayer game - we need to wake up, and we don't have the
    // inbedchatscreen with a button

    if (button == 1 &&
        (isSleeping() && level != nullptr && level->isClientSide)) {
        if (lastClickState == lastClick_oldRepeat) return false;

        std::shared_ptr<MultiplayerLocalPlayer> mplp =
            std::dynamic_pointer_cast<MultiplayerLocalPlayer>(
                shared_from_this());

        if (mplp && mplp->connection) mplp->StopSleeping();
    }
    // 4J Stu - We should not accept any input while asleep, except the above to
    // wake up
    if (isSleeping() && level != nullptr && level->isClientSide) {
        return false;
    }

    std::shared_ptr<ItemInstance> oldItem = inventory->getSelected();

    if (minecraft->hitResult == nullptr) {
        if (button == 0 &&
            minecraft->localgameModes[GetXboxPad()]->hasMissTime())
            missTime = 10;
    } else if (minecraft->hitResult->type == HitResult::ENTITY) {
        if (button == 0) {
            minecraft->gameMode->attack(minecraft->localplayers[GetXboxPad()],
                                        minecraft->hitResult->entity);
        }
        if (button == 1) {
            // 4J-PB - if we milk a cow here, and end up with a bucket of milk,
            // the if (mayUse && button == 1) further down will then empty our
            // bucket if we're pointing at a tile It looks like interact really
            // should be returning a result so we can check this, but it's
            // possibly just the milk bucket that causes a problem

            if (minecraft->hitResult->entity->GetType() == eTYPE_COW) {
                // If I have an empty bucket in my hand, it's going to be filled
                // with milk, so turn off mayUse
                std::shared_ptr<ItemInstance> item = inventory->getSelected();
                if (item && (item->id == Item::bucket_empty_Id)) {
                    mayUse = false;
                }
            }
            if (minecraft->gameMode->interact(
                    minecraft->localplayers[GetXboxPad()],
                    minecraft->hitResult->entity)) {
                mayUse = false;
            }
        }
    } else if (minecraft->hitResult->type == HitResult::TILE) {
        int x = minecraft->hitResult->x;
        int y = minecraft->hitResult->y;
        int z = minecraft->hitResult->z;
        int face = minecraft->hitResult->f;

        if (button == 0) {
            // 4J - addition to stop layer mining out of the top or bottom of
            // the world 4J Stu - Allow this for The End
            if (!((y == 0) || ((y == 127) && level->dimension->hasCeiling)) ||
                level->dimension->id == 1) {
                minecraft->gameMode->startDestroyBlock(x, y, z,
                                                       minecraft->hitResult->f);
            }
        } else {
            std::shared_ptr<ItemInstance> item = oldItem;
            int oldCount = item != nullptr ? item->count : 0;
            bool usedItem = false;
            if (minecraft->gameMode->useItemOn(
                    minecraft->localplayers[GetXboxPad()], level, item, x, y, z,
                    face, &minecraft->hitResult->pos, false, &usedItem)) {
                // Presume that if we actually used the held item, then we've
                // placed it
                if (usedItem) {
                    returnItemPlaced = true;
                }
                mayUse = false;
                // Log::info("Player %d is swinging\n",GetXboxPad());
                swing();
            }
            if (item == nullptr) {
                return false;
            }

            if (item->count == 0) {
                inventory->items[inventory->selected] = nullptr;
            } else if (item->count != oldCount ||
                       minecraft->localgameModes[GetXboxPad()]
                           ->hasInfiniteItems()) {
                minecraft->gameRenderer->itemInHandRenderer->itemPlaced();
            }
        }
    }

    if (mayUse && button == 1) {
        std::shared_ptr<ItemInstance> item = inventory->getSelected();
        if (item != nullptr) {
            if (minecraft->gameMode->useItem(
                    minecraft->localplayers[GetXboxPad()], level, item)) {
                minecraft->gameRenderer->itemInHandRenderer->itemUsed();
            }
        }
    }
    return returnItemPlaced;
}

void LocalPlayer::updateRichPresence() {
    if ((m_iPad != -1) /* && !ui.GetMenuDisplayed(m_iPad)*/) {
        std::shared_ptr<ItemInstance> selectedItem = inventory->getSelected();
        if (selectedItem != nullptr &&
            selectedItem->id == Item::fishingRod_Id) {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_FISHING);
        } else if (selectedItem != nullptr &&
                   selectedItem->id == Item::map_Id) {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_MAP);
        } else if ((riding != nullptr) && riding->instanceof (eTYPE_MINECART)) {
            gameServices().setRichPresenceContext(
                m_iPad, CONTEXT_GAME_STATE_RIDING_MINECART);
        } else if ((riding != nullptr) && riding->instanceof (eTYPE_BOAT)) {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_BOATING);
        } else if ((riding != nullptr) && riding->instanceof (eTYPE_PIG)) {
            gameServices().setRichPresenceContext(
                m_iPad, CONTEXT_GAME_STATE_RIDING_PIG);
        } else if (this->dimension == -1) {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_NETHER);
        } else if (minecraft->soundEngine->GetIsPlayingStreamingCDMusic()) {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_CD);
        } else {
            gameServices().setRichPresenceContext(m_iPad,
                                                  CONTEXT_GAME_STATE_BLANK);
        }
    }
}

// 4J Stu - Added for telemetry
void LocalPlayer::SetSessionTimerStart(void) {
    m_sessionTimeStart = gameServices().getAppTime();
    m_dimensionTimeStart = m_sessionTimeStart;
}

float LocalPlayer::getSessionTimer(void) {
    return gameServices().getAppTime() - m_sessionTimeStart;
}

float LocalPlayer::getAndResetChangeDimensionTimer() {
    float appTime = gameServices().getAppTime();
    float returnVal = appTime - m_dimensionTimeStart;
    m_dimensionTimeStart = appTime;
    return returnVal;
}

void LocalPlayer::handleCollectItem(std::shared_ptr<ItemInstance> item) {
    if (item != nullptr) {
        unsigned int itemCountAnyAux = 0;
        unsigned int itemCountThisAux = 0;
        for (unsigned int k = 0; k < inventory->items.size(); ++k) {
            if (inventory->items[k] != nullptr) {
                // do they have the item
                if (inventory->items[k]->id == item->id) {
                    unsigned int quantity = inventory->items[k]->GetCount();

                    itemCountAnyAux += quantity;

                    if (inventory->items[k]->getAuxValue() ==
                        item->getAuxValue()) {
                        itemCountThisAux += quantity;
                    }
                }
            }
        }
        TutorialMode* gameMode =
            (TutorialMode*)minecraft->localgameModes[m_iPad];
        gameMode->getTutorial()->onTake(item, itemCountAnyAux,
                                        itemCountThisAux);
    }

    if (ui.IsContainerMenuDisplayed(m_iPad)) {
        ui.HandleInventoryUpdated(m_iPad);
    }
}

void LocalPlayer::SetPlayerAdditionalModelParts(
    std::vector<ModelPart*>& pAdditionalModelParts) {
    m_pAdditionalModelParts = pAdditionalModelParts;
}
