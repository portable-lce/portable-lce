#include "PreStitchedTextureMap.h"

#include <format>
#include <utility>

#include "SimpleIcon.h"
#include "StitchedTexture.h"
#include "Texture.h"
#include "TextureManager.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/LevelRenderer.h"
#include "minecraft/client/renderer/entity/EntityRenderDispatcher.h"
#include "minecraft/client/renderer/texture/custom/ClockTexture.h"
#include "minecraft/client/renderer/texture/custom/CompassTexture.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/item/Item.h"
#include "minecraft/world/level/tile/Tile.h"

class Stitcher;
class TextureHolder;

const std::string PreStitchedTextureMap::NAME_MISSING_TEXTURE = "missingno";

PreStitchedTextureMap::PreStitchedTextureMap(int type, const std::string& name,
                                             const std::string& path,
                                             BufferedImage* missingTexture,
                                             bool mipmap)
    : iconType(type), name(name), path(path), extension(".png") {
    this->missingTexture = missingTexture;

    // 4J Initialisers
    missingPosition = nullptr;
    stitchResult = nullptr;

    m_mipMap = mipmap;
    missingPosition = (StitchedTexture*)(new SimpleIcon(
        NAME_MISSING_TEXTURE, NAME_MISSING_TEXTURE, 0, 0, 1, 1));
}

void PreStitchedTextureMap::stitch() {
    // Animated StitchedTextures store a vector of textures for each frame of
    // the animation. Free any pre-existing ones here.
    for (auto it = animatedTextures.begin(); it != animatedTextures.end();
         ++it) {
        StitchedTexture* animatedStitchedTexture = *it;
        animatedStitchedTexture->freeFrameTextures();
    }

    loadUVs();

    if (iconType == Icon::TYPE_TERRAIN) {
        // for (Tile tile : Tile.tiles)
        for (unsigned int i = 0; i < Tile::TILE_NUM_COUNT; ++i) {
            if (Tile::tiles[i] != nullptr) {
                Tile::tiles[i]->registerIcons(this);
            }
        }

        Minecraft::GetInstance()->levelRenderer->registerTextures(this);
        EntityRenderDispatcher::instance->registerTerrainTextures(this);
    }

    // for (Item item : Item.items)
    for (unsigned int i = 0; i < Item::ITEM_NUM_COUNT; ++i) {
        Item* item = Item::items[i];
        if (item != nullptr && item->getIconType() == iconType) {
            item->registerIcons(this);
        }
    }

    // Collection bucket for multiple frames per texture
    std::unordered_map<TextureHolder*, std::vector<Texture*>*>
        textures;  // = new HashMap<TextureHolder, List<Texture>>();

    Stitcher* stitcher = TextureManager::getInstance()->createStitcher(name);

    animatedTextures.clear();

    // Create the final image
    std::string filename = name + extension;

    TexturePack* texturePack = Minecraft::GetInstance()->skins->getSelected();
    // try {
    int mode = Texture::TM_DYNAMIC;
    int clamp = Texture::WM_WRAP;  // 4J Stu - Don't clamp as it causes issues
                                   // with how we signal non-mipmmapped textures
                                   // to the pixel shader //Texture::WM_CLAMP;
    int minFilter = Texture::TFLT_NEAREST;
    int magFilter = Texture::TFLT_NEAREST;

    std::string drive = "";

    // 4J-PB - need to check for BD patched files
    if (texturePack->hasFile("res/" + filename, false)) {
        drive = texturePack->getPath(true);
    } else {
        drive = Minecraft::GetInstance()->skins->getDefault()->getPath(true);
        texturePack = Minecraft::GetInstance()->skins->getDefault();
    }

    // BufferedImage *image = new BufferedImage(texturePack->getResource("/" +
    // filename),false,true,drive);
    // //ImageIO::read(texturePack->getResource("/" + filename));
    BufferedImage* image =
        texturePack->getImageResource(filename, false, true, drive);
    int height = image->getHeight();
    int width = image->getWidth();

    if (stitchResult != nullptr) {
        TextureManager::getInstance()->unregisterTexture(name, stitchResult);
        delete stitchResult;
    }
    stitchResult = TextureManager::getInstance()->createTexture(
        name, Texture::TM_DYNAMIC, width, height, Texture::TFMT_RGBA, m_mipMap);
    stitchResult->transferFromImage(image);
    delete image;
    TextureManager::getInstance()->registerName(name, stitchResult);
    // stitchResult = stitcher->constructTexture(m_mipMap);

    for (auto it = texturesByName.begin(); it != texturesByName.end(); ++it) {
        StitchedTexture* preStitched = (StitchedTexture*)it->second;

        int x = preStitched->getU0() * stitchResult->getWidth();
        int y = preStitched->getV0() * stitchResult->getHeight();
        int width = (preStitched->getU1() * stitchResult->getWidth()) - x;
        int height = (preStitched->getV1() * stitchResult->getHeight()) - y;

        preStitched->init(stitchResult, nullptr, x, y, width, height, false);
    }

    for (auto it = texturesByName.begin(); it != texturesByName.end(); ++it) {
        StitchedTexture* preStitched = (StitchedTexture*)(it->second);

        makeTextureAnimated(texturePack, preStitched);
    }
    // missingPosition = (StitchedTexture
    // *)texturesByName.find(NAME_MISSING_TEXTURE)->second;

    stitchResult->writeAsPNG("debug.stitched_" + name + ".png");
    stitchResult->updateOnGPU();
}

void PreStitchedTextureMap::makeTextureAnimated(TexturePack* texturePack,
                                                StitchedTexture* tex) {
    if (!tex->hasOwnData()) {
        animatedTextures.push_back(tex);
        return;
    }

    std::string textureFileName = tex->m_fileName;

    std::string animString =
        texturePack->getAnimationString(textureFileName, path, true);

    if (!animString.empty()) {
        std::string filename = path + textureFileName + extension;

        // TODO: [EB] Put the frames into a proper object, not this inside out
        // hack
        std::vector<Texture*>* frames =
            TextureManager::getInstance()->createTextures(filename, m_mipMap);
        if (frames == nullptr || frames->empty()) {
            return;  // Couldn't load a texture, skip it
        }

        Texture* first = frames->at(0);

#if !defined(_CONTENT_PACKAGE)
        if (first->getWidth() != tex->getWidth() ||
            first->getHeight() != tex->getHeight()) {
            Log::info("%s - first w - %d, h - %d, tex w - %d, h - %d\n",
                      textureFileName.c_str(), first->getWidth(),
                      tex->getWidth(), first->getHeight(), tex->getHeight());
            assert(0);
        }
#endif

        tex->init(stitchResult, frames, tex->getX(), tex->getY(),
                  first->getWidth(), first->getHeight(), false);

        if (frames->size() > 1) {
            animatedTextures.push_back(tex);

            tex->loadAnimationFrames(animString);
        }
    }
}

StitchedTexture* PreStitchedTextureMap::getTexture(const std::string& name) {
#if !defined(_CONTENT_PACKAGE)
    Log::info("Not implemented!\n");
    assert(0);
#endif
    return nullptr;
}

void PreStitchedTextureMap::cycleAnimationFrames() {
    // for (StitchedTexture texture : animatedTextures)
    for (auto it = animatedTextures.begin(); it != animatedTextures.end();
         ++it) {
        StitchedTexture* texture = *it;
        texture->cycleFrames();
    }
}

Texture* PreStitchedTextureMap::getStitchedTexture() { return stitchResult; }

// 4J Stu - register is a reserved keyword in C++
Icon* PreStitchedTextureMap::registerIcon(const std::string& name) {
    Icon* result = nullptr;
    if (name.empty()) {
        Log::info("Don't register nullptr\n");
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
        result = missingPosition;
        // new RuntimeException("Don't register null!").printStackTrace();
    }

    auto it = texturesByName.find(name);
    if (it != texturesByName.end()) result = it->second;

    if (result == nullptr) {
#if !defined(_CONTENT_PACKAGE)
        Log::info("Could not find uv data for icon %s\n", name.c_str());
        assert(0);
#endif
        result = missingPosition;
    }

    return result;
}

int PreStitchedTextureMap::getIconType() { return iconType; }

Icon* PreStitchedTextureMap::getMissingIcon() { return missingPosition; }

#define ADD_ICON(row, column, name)                                       \
    (texturesByName[name] =                                               \
         new SimpleIcon(name, name, horizRatio * column, vertRatio * row, \
                        horizRatio * (column + 1), vertRatio * (row + 1)));
#define ADD_ICON_WITH_NAME(row, column, name, filename)                       \
    (texturesByName[name] =                                                   \
         new SimpleIcon(name, filename, horizRatio * column, vertRatio * row, \
                        horizRatio * (column + 1), vertRatio * (row + 1)));
#define ADD_ICON_SIZE(row, column, name, height, width)    \
    (texturesByName[name] = new SimpleIcon(                \
         name, name, horizRatio * column, vertRatio * row, \
         horizRatio * (column + width), vertRatio * (row + height)));

void PreStitchedTextureMap::loadUVs() {
    if (!texturesByName.empty()) {
        // 4J Stu - We only need to populate this once at the moment as we have
        // hardcoded positions for each texture If we ever load that
        // dynamically, be aware that the Icon objects could currently be being
        // used by the GameRenderer::runUpdate thread
        return;
    }

    for (auto it = texturesByName.begin(); it != texturesByName.end(); ++it) {
        delete it->second;
    }
    texturesByName.clear();

    if (iconType != Icon::TYPE_TERRAIN) {
        float horizRatio = 1.0f / 16.0f;
        float vertRatio = 1.0f / 16.0f;

        ADD_ICON(0, 0, "helmetCloth")
        ADD_ICON(0, 1, "helmetChain")
        ADD_ICON(0, 2, "helmetIron")
        ADD_ICON(0, 3, "helmetDiamond")
        ADD_ICON(0, 4, "helmetGold")
        ADD_ICON(0, 5, "flintAndSteel")
        ADD_ICON(0, 6, "flint")
        ADD_ICON(0, 7, "coal")
        ADD_ICON(0, 8, "string")
        ADD_ICON(0, 9, "seeds")
        ADD_ICON(0, 10, "apple")
        ADD_ICON(0, 11, "appleGold")
        ADD_ICON(0, 12, "egg")
        ADD_ICON(0, 13, "sugar")
        ADD_ICON(0, 14, "snowball")
        ADD_ICON(0, 15, "slot_empty_helmet")

        ADD_ICON(1, 0, "chestplateCloth")
        ADD_ICON(1, 1, "chestplateChain")
        ADD_ICON(1, 2, "chestplateIron")
        ADD_ICON(1, 3, "chestplateDiamond")
        ADD_ICON(1, 4, "chestplateGold")
        ADD_ICON(1, 5, "bow")
        ADD_ICON(1, 6, "brick")
        ADD_ICON(1, 7, "ingotIron")
        ADD_ICON(1, 8, "feather")
        ADD_ICON(1, 9, "wheat")
        ADD_ICON(1, 10, "painting")
        ADD_ICON(1, 11, "reeds")
        ADD_ICON(1, 12, "bone")
        ADD_ICON(1, 13, "cake")
        ADD_ICON(1, 14, "slimeball")
        ADD_ICON(1, 15, "slot_empty_chestplate")

        ADD_ICON(2, 0, "leggingsCloth")
        ADD_ICON(2, 1, "leggingsChain")
        ADD_ICON(2, 2, "leggingsIron")
        ADD_ICON(2, 3, "leggingsDiamond")
        ADD_ICON(2, 4, "leggingsGold")
        ADD_ICON(2, 5, "arrow")
        ADD_ICON(2, 6, "quiver")
        ADD_ICON(2, 7, "ingotGold")
        ADD_ICON(2, 8, "sulphur")
        ADD_ICON(2, 9, "bread")
        ADD_ICON(2, 10, "sign")
        ADD_ICON(2, 11, "doorWood")
        ADD_ICON(2, 12, "doorIron")
        ADD_ICON(2, 13, "bed")
        ADD_ICON(2, 14, "fireball")
        ADD_ICON(2, 15, "slot_empty_leggings")

        ADD_ICON(3, 0, "bootsCloth")
        ADD_ICON(3, 1, "bootsChain")
        ADD_ICON(3, 2, "bootsIron")
        ADD_ICON(3, 3, "bootsDiamond")
        ADD_ICON(3, 4, "bootsGold")
        ADD_ICON(3, 5, "stick")
        ADD_ICON(3, 6, "compass")
        ADD_ICON(3, 7, "diamond")
        ADD_ICON(3, 8, "redstone")
        ADD_ICON(3, 9, "clay")
        ADD_ICON(3, 10, "paper")
        ADD_ICON(3, 11, "book")
        ADD_ICON(3, 12, "map")
        ADD_ICON(3, 13, "seeds_pumpkin")
        ADD_ICON(3, 14, "seeds_melon")
        ADD_ICON(3, 15, "slot_empty_boots")

        ADD_ICON(4, 0, "swordWood")
        ADD_ICON(4, 1, "swordStone")
        ADD_ICON(4, 2, "swordIron")
        ADD_ICON(4, 3, "swordDiamond")
        ADD_ICON(4, 4, "swordGold")
        ADD_ICON(4, 5, "fishingRod_uncast")
        ADD_ICON(4, 6, "clock")
        ADD_ICON(4, 7, "bowl")
        ADD_ICON(4, 8, "mushroomStew")
        ADD_ICON(4, 9, "yellowDust")
        ADD_ICON(4, 10, "bucket")
        ADD_ICON(4, 11, "bucketWater")
        ADD_ICON(4, 12, "bucketLava")
        ADD_ICON(4, 13, "milk")
        ADD_ICON(4, 14, "dyePowder_black")
        ADD_ICON(4, 15, "dyePowder_gray")

        ADD_ICON(5, 0, "shovelWood")
        ADD_ICON(5, 1, "shovelStone")
        ADD_ICON(5, 2, "shovelIron")
        ADD_ICON(5, 3, "shovelDiamond")
        ADD_ICON(5, 4, "shovelGold")
        ADD_ICON(5, 5, "fishingRod_cast")
        ADD_ICON(5, 6, "diode")
        ADD_ICON(5, 7, "porkchopRaw")
        ADD_ICON(5, 8, "porkchopCooked")
        ADD_ICON(5, 9, "fishRaw")
        ADD_ICON(5, 10, "fishCooked")
        ADD_ICON(5, 11, "rottenFlesh")
        ADD_ICON(5, 12, "cookie")
        ADD_ICON(5, 13, "shears")
        ADD_ICON(5, 14, "dyePowder_red")
        ADD_ICON(5, 15, "dyePowder_pink")

        ADD_ICON(6, 0, "pickaxeWood")
        ADD_ICON(6, 1, "pickaxeStone")
        ADD_ICON(6, 2, "pickaxeIron")
        ADD_ICON(6, 3, "pickaxeDiamond")
        ADD_ICON(6, 4, "pickaxeGold")
        ADD_ICON(6, 5, "bow_pull_0")
        ADD_ICON(6, 6, "carrotOnAStick")
        ADD_ICON(6, 7, "leather")
        ADD_ICON(6, 8, "saddle")
        ADD_ICON(6, 9, "beefRaw")
        ADD_ICON(6, 10, "beefCooked")
        ADD_ICON(6, 11, "enderPearl")
        ADD_ICON(6, 12, "blazeRod")
        ADD_ICON(6, 13, "melon")
        ADD_ICON(6, 14, "dyePowder_green")
        ADD_ICON(6, 15, "dyePowder_lime")

        ADD_ICON(7, 0, "hatchetWood")
        ADD_ICON(7, 1, "hatchetStone")
        ADD_ICON(7, 2, "hatchetIron")
        ADD_ICON(7, 3, "hatchetDiamond")
        ADD_ICON(7, 4, "hatchetGold")
        ADD_ICON(7, 5, "bow_pull_1")
        ADD_ICON(7, 6, "potatoBaked")
        ADD_ICON(7, 7, "potato")
        ADD_ICON(7, 8, "carrots")
        ADD_ICON(7, 9, "chickenRaw")
        ADD_ICON(7, 10, "chickenCooked")
        ADD_ICON(7, 11, "ghastTear")
        ADD_ICON(7, 12, "goldNugget")
        ADD_ICON(7, 13, "netherStalkSeeds")
        ADD_ICON(7, 14, "dyePowder_brown")
        ADD_ICON(7, 15, "dyePowder_yellow")

        ADD_ICON(8, 0, "hoeWood")
        ADD_ICON(8, 1, "hoeStone")
        ADD_ICON(8, 2, "hoeIron")
        ADD_ICON(8, 3, "hoeDiamond")
        ADD_ICON(8, 4, "hoeGold")
        ADD_ICON(8, 5, "bow_pull_2")
        ADD_ICON(8, 6, "potatoPoisonous")
        ADD_ICON(8, 7, "minecart")
        ADD_ICON(8, 8, "boat")
        ADD_ICON(8, 9, "speckledMelon")
        ADD_ICON(8, 10, "fermentedSpiderEye")
        ADD_ICON(8, 11, "spiderEye")
        ADD_ICON(8, 12, "potion")
        ADD_ICON(8, 12, "glassBottle")  // Same as potion
        ADD_ICON(8, 13, "potion_contents")
        ADD_ICON(8, 14, "dyePowder_blue")
        ADD_ICON(8, 15, "dyePowder_light_blue")

        ADD_ICON(9, 0, "helmetCloth_overlay")
        // ADD_ICON(9,		1,	"unused")
        ADD_ICON(9, 2, "iron_horse_armor")
        ADD_ICON(9, 3, "diamond_horse_armor")
        ADD_ICON(9, 4, "gold_horse_armor")
        ADD_ICON(9, 5, "comparator")
        ADD_ICON(9, 6, "carrotGolden")
        ADD_ICON(9, 7, "minecart_chest")
        ADD_ICON(9, 8, "pumpkinPie")
        ADD_ICON(9, 9, "monsterPlacer")
        ADD_ICON(9, 10, "potion_splash")
        ADD_ICON(9, 11, "eyeOfEnder")
        ADD_ICON(9, 12, "cauldron")
        ADD_ICON(9, 13, "blazePowder")
        ADD_ICON(9, 14, "dyePowder_purple")
        ADD_ICON(9, 15, "dyePowder_magenta")

        ADD_ICON(10, 0, "chestplateCloth_overlay")
        // ADD_ICON(10,	1,	"unused")
        // ADD_ICON(10,	2,	"unused")
        ADD_ICON(10, 3, "name_tag")
        ADD_ICON(10, 4, "lead")
        ADD_ICON(10, 5, "netherbrick")
        // ADD_ICON(10,	6,	"unused")
        ADD_ICON(10, 7, "minecart_furnace")
        ADD_ICON(10, 8, "charcoal")
        ADD_ICON(10, 9, "monsterPlacer_overlay")
        ADD_ICON(10, 10, "ruby")
        ADD_ICON(10, 11, "expBottle")
        ADD_ICON(10, 12, "brewingStand")
        ADD_ICON(10, 13, "magmaCream")
        ADD_ICON(10, 14, "dyePowder_cyan")
        ADD_ICON(10, 15, "dyePowder_orange")

        ADD_ICON(11, 0, "leggingsCloth_overlay")
        // ADD_ICON(11,	1,	"unused")
        // ADD_ICON(11,	2,	"unused")
        // ADD_ICON(11,	3,	"unused")
        // ADD_ICON(11,	4,	"unused")
        // ADD_ICON(11,	5,	"unused")
        // ADD_ICON(11,	6,	"unused")
        ADD_ICON(11, 7, "minecart_hopper")
        ADD_ICON(11, 8, "hopper")
        ADD_ICON(11, 9, "nether_star")
        ADD_ICON(11, 10, "emerald")
        ADD_ICON(11, 11, "writingBook")
        ADD_ICON(11, 12, "writtenBook")
        ADD_ICON(11, 13, "flowerPot")
        ADD_ICON(11, 14, "dyePowder_silver")
        ADD_ICON(11, 15, "dyePowder_white")

        ADD_ICON(12, 0, "bootsCloth_overlay")
        // ADD_ICON(12,	1,	"unused")
        // ADD_ICON(12,	2,	"unused")
        // ADD_ICON(12,	3,	"unused")
        // ADD_ICON(12,	4,	"unused")
        // ADD_ICON(12,	5,	"unused")
        // ADD_ICON(12,	6,	"unused")
        ADD_ICON(12, 7, "minecart_tnt")
        // ADD_ICON(12,	8,	"unused")
        ADD_ICON(12, 9, "fireworks")
        ADD_ICON(12, 10, "fireworks_charge")
        ADD_ICON(12, 11, "fireworks_charge_overlay")
        ADD_ICON(12, 12, "netherquartz")
        ADD_ICON(12, 13, "map_empty")
        ADD_ICON(12, 14, "frame")
        ADD_ICON(12, 15, "enchantedBook")

        ADD_ICON(14, 0, "skull_skeleton")
        ADD_ICON(14, 1, "skull_wither")
        ADD_ICON(14, 2, "skull_zombie")
        ADD_ICON(14, 3, "skull_char")
        ADD_ICON(14, 4, "skull_creeper")
        // ADD_ICON(14,	5,	"unused")
        // ADD_ICON(14,	6,	"unused")
        ADD_ICON_WITH_NAME(14, 7, "compassP0", "compass")   // 4J Added
        ADD_ICON_WITH_NAME(14, 8, "compassP1", "compass")   // 4J Added
        ADD_ICON_WITH_NAME(14, 9, "compassP2", "compass")   // 4J Added
        ADD_ICON_WITH_NAME(14, 10, "compassP3", "compass")  // 4J Added
        ADD_ICON_WITH_NAME(14, 11, "clockP0", "clock")      // 4J Added
        ADD_ICON_WITH_NAME(14, 12, "clockP1", "clock")      // 4J Added
        ADD_ICON_WITH_NAME(14, 13, "clockP2", "clock")      // 4J Added
        ADD_ICON_WITH_NAME(14, 14, "clockP3", "clock")      // 4J Added
        ADD_ICON(14, 15, "dragonFireball")

        ADD_ICON(15, 0, "record_13")
        ADD_ICON(15, 1, "record_cat")
        ADD_ICON(15, 2, "record_blocks")
        ADD_ICON(15, 3, "record_chirp")
        ADD_ICON(15, 4, "record_far")
        ADD_ICON(15, 5, "record_mall")
        ADD_ICON(15, 6, "record_mellohi")
        ADD_ICON(15, 7, "record_stal")
        ADD_ICON(15, 8, "record_strad")
        ADD_ICON(15, 9, "record_ward")
        ADD_ICON(15, 10, "record_11")
        ADD_ICON(15, 11, "record_where are we now")

        // Special cases
        ClockTexture* dataClock = new ClockTexture();
        Icon* oldClock = texturesByName["clock"];
        dataClock->initUVs(oldClock->getU0(), oldClock->getV0(),
                           oldClock->getU1(), oldClock->getV1());
        delete oldClock;
        texturesByName["clock"] = dataClock;

        ClockTexture* clock = new ClockTexture(0, dataClock);
        oldClock = texturesByName["clockP0"];
        clock->initUVs(oldClock->getU0(), oldClock->getV0(), oldClock->getU1(),
                       oldClock->getV1());
        delete oldClock;
        texturesByName["clockP0"] = clock;

        clock = new ClockTexture(1, dataClock);
        oldClock = texturesByName["clockP1"];
        clock->initUVs(oldClock->getU0(), oldClock->getV0(), oldClock->getU1(),
                       oldClock->getV1());
        delete oldClock;
        texturesByName["clockP1"] = clock;

        clock = new ClockTexture(2, dataClock);
        oldClock = texturesByName["clockP2"];
        clock->initUVs(oldClock->getU0(), oldClock->getV0(), oldClock->getU1(),
                       oldClock->getV1());
        delete oldClock;
        texturesByName["clockP2"] = clock;

        clock = new ClockTexture(3, dataClock);
        oldClock = texturesByName["clockP3"];
        clock->initUVs(oldClock->getU0(), oldClock->getV0(), oldClock->getU1(),
                       oldClock->getV1());
        delete oldClock;
        texturesByName["clockP3"] = clock;

        CompassTexture* dataCompass = new CompassTexture();
        Icon* oldCompass = texturesByName["compass"];
        dataCompass->initUVs(oldCompass->getU0(), oldCompass->getV0(),
                             oldCompass->getU1(), oldCompass->getV1());
        delete oldCompass;
        texturesByName["compass"] = dataCompass;

        CompassTexture* compass = new CompassTexture(0, dataCompass);
        oldCompass = texturesByName["compassP0"];
        compass->initUVs(oldCompass->getU0(), oldCompass->getV0(),
                         oldCompass->getU1(), oldCompass->getV1());
        delete oldCompass;
        texturesByName["compassP0"] = compass;

        compass = new CompassTexture(1, dataCompass);
        oldCompass = texturesByName["compassP1"];
        compass->initUVs(oldCompass->getU0(), oldCompass->getV0(),
                         oldCompass->getU1(), oldCompass->getV1());
        delete oldCompass;
        texturesByName["compassP1"] = compass;

        compass = new CompassTexture(2, dataCompass);
        oldCompass = texturesByName["compassP2"];
        compass->initUVs(oldCompass->getU0(), oldCompass->getV0(),
                         oldCompass->getU1(), oldCompass->getV1());
        delete oldCompass;
        texturesByName["compassP2"] = compass;

        compass = new CompassTexture(3, dataCompass);
        oldCompass = texturesByName["compassP3"];
        compass->initUVs(oldCompass->getU0(), oldCompass->getV0(),
                         oldCompass->getU1(), oldCompass->getV1());
        delete oldCompass;
        texturesByName["compassP3"] = compass;
    } else {
        float horizRatio = 1.0f / 16.0f;
        float vertRatio = 1.0f / 32.0f;

        ADD_ICON(0, 0, "grass_top")
        texturesByName["grass_top"]->setFlags(
            Icon::IS_GRASS_TOP);  // 4J added for faster determination of
                                  // texture type in tesselation
        ADD_ICON(0, 1, "stone")
        ADD_ICON(0, 2, "dirt")
        ADD_ICON(0, 3, "grass_side")
        texturesByName["grass_side"]->setFlags(
            Icon::IS_GRASS_SIDE);  // 4J added for faster determination of
                                   // texture type in tesselation
        ADD_ICON(0, 4, "planks_oak")
        ADD_ICON(0, 5, "stoneslab_side")
        ADD_ICON(0, 6, "stoneslab_top")
        ADD_ICON(0, 7, "brick")
        ADD_ICON(0, 8, "tnt_side")
        ADD_ICON(0, 9, "tnt_top")
        ADD_ICON(0, 10, "tnt_bottom")
        ADD_ICON(0, 11, "web")
        ADD_ICON(0, 12, "flower_rose")
        ADD_ICON(0, 13, "flower_dandelion")
        ADD_ICON(0, 14, "portal")
        ADD_ICON(0, 15, "sapling")

        ADD_ICON(1, 0, "cobblestone");
        ADD_ICON(1, 1, "bedrock");
        ADD_ICON(1, 2, "sand");
        ADD_ICON(1, 3, "gravel");
        ADD_ICON(1, 4, "log_oak");
        ADD_ICON(1, 5, "log_oak_top");
        ADD_ICON(1, 6, "iron_block");
        ADD_ICON(1, 7, "gold_block");
        ADD_ICON(1, 8, "diamond_block");
        ADD_ICON(1, 9, "emerald_block");
        ADD_ICON(1, 10, "redstone_block");
        ADD_ICON(1, 11, "dropper_front_horizontal");
        ADD_ICON(1, 12, "mushroom_red");
        ADD_ICON(1, 13, "mushroom_brown");
        ADD_ICON(1, 14, "sapling_jungle");
        ADD_ICON(1, 15, "fire_0");

        ADD_ICON(2, 0, "gold_ore");
        ADD_ICON(2, 1, "iron_ore");
        ADD_ICON(2, 2, "coal_ore");
        ADD_ICON(2, 3, "bookshelf");
        ADD_ICON(2, 4, "cobblestone_mossy");
        ADD_ICON(2, 5, "obsidian");
        ADD_ICON(2, 6, "grass_side_overlay");
        ADD_ICON(2, 7, "tallgrass");
        ADD_ICON(2, 8, "dispenser_front_vertical");
        ADD_ICON(2, 9, "beacon");
        ADD_ICON(2, 10, "dropper_front_vertical");
        ADD_ICON(2, 11, "workbench_top");
        ADD_ICON(2, 12, "furnace_front");
        ADD_ICON(2, 13, "furnace_side");
        ADD_ICON(2, 14, "dispenser_front");
        ADD_ICON(2, 15, "fire_1");

        ADD_ICON(3, 0, "sponge");
        ADD_ICON(3, 1, "glass");
        ADD_ICON(3, 2, "diamond_ore");
        ADD_ICON(3, 3, "redstone_ore");
        ADD_ICON(3, 4, "leaves");
        ADD_ICON(3, 5, "leaves_opaque");
        ADD_ICON(3, 6, "stonebrick");
        ADD_ICON(3, 7, "deadbush");
        ADD_ICON(3, 8, "fern");
        ADD_ICON(3, 9, "daylight_detector_top");
        ADD_ICON(3, 10, "daylight_detector_side");
        ADD_ICON(3, 11, "workbench_side");
        ADD_ICON(3, 12, "workbench_front");
        ADD_ICON(3, 13, "furnace_front_lit");
        ADD_ICON(3, 14, "furnace_top");
        ADD_ICON(3, 15, "sapling_spruce");

        ADD_ICON(4, 0, "wool_colored_white");
        ADD_ICON(4, 1, "mob_spawner");
        ADD_ICON(4, 2, "snow");
        ADD_ICON(4, 3, "ice");
        ADD_ICON(4, 4, "snow_side");
        ADD_ICON(4, 5, "cactus_top");
        ADD_ICON(4, 6, "cactus_side");
        ADD_ICON(4, 7, "cactus_bottom");
        ADD_ICON(4, 8, "clay");
        ADD_ICON(4, 9, "reeds");
        ADD_ICON(4, 10, "jukebox_side");
        ADD_ICON(4, 11, "jukebox_top");
        ADD_ICON(4, 12, "waterlily");
        ADD_ICON(4, 13, "mycel_side");
        ADD_ICON(4, 14, "mycel_top");
        ADD_ICON(4, 15, "sapling_birch");

        ADD_ICON(5, 0, "torch_on");
        ADD_ICON(5, 1, "door_wood_upper");
        ADD_ICON(5, 2, "door_iron_upper");
        ADD_ICON(5, 3, "ladder");
        ADD_ICON(5, 4, "trapdoor");
        ADD_ICON(5, 5, "iron_bars");
        ADD_ICON(5, 6, "farmland_wet");
        ADD_ICON(5, 7, "farmland_dry");
        ADD_ICON(5, 8, "crops_0");
        ADD_ICON(5, 9, "crops_1");
        ADD_ICON(5, 10, "crops_2");
        ADD_ICON(5, 11, "crops_3");
        ADD_ICON(5, 12, "crops_4");
        ADD_ICON(5, 13, "crops_5");
        ADD_ICON(5, 14, "crops_6");
        ADD_ICON(5, 15, "crops_7");

        ADD_ICON(6, 0, "lever");
        ADD_ICON(6, 1, "door_wood_lower");
        ADD_ICON(6, 2, "door_iron_lower");
        ADD_ICON(6, 3, "redstone_torch_on");
        ADD_ICON(6, 4, "stonebrick_mossy");
        ADD_ICON(6, 5, "stonebrick_cracked");
        ADD_ICON(6, 6, "pumpkin_top");
        ADD_ICON(6, 7, "netherrack");
        ADD_ICON(6, 8, "soul_sand");
        ADD_ICON(6, 9, "glowstone");
        ADD_ICON(6, 10, "piston_top_sticky");
        ADD_ICON(6, 11, "piston_top");
        ADD_ICON(6, 12, "piston_side");
        ADD_ICON(6, 13, "piston_bottom");
        ADD_ICON(6, 14, "piston_inner_top");
        ADD_ICON(6, 15, "stem_straight");

        ADD_ICON(7, 0, "rail_normal_turned");
        ADD_ICON(7, 1, "wool_colored_black");
        ADD_ICON(7, 2, "wool_colored_gray");
        ADD_ICON(7, 3, "redstone_torch_off");
        ADD_ICON(7, 4, "log_spruce");
        ADD_ICON(7, 5, "log_birch");
        ADD_ICON(7, 6, "pumpkin_side");
        ADD_ICON(7, 7, "pumpkin_face_off");
        ADD_ICON(7, 8, "pumpkin_face_on");
        ADD_ICON(7, 9, "cake_top");
        ADD_ICON(7, 10, "cake_side");
        ADD_ICON(7, 11, "cake_inner");
        ADD_ICON(7, 12, "cake_bottom");
        ADD_ICON(7, 13, "mushroom_block_skin_red");
        ADD_ICON(7, 14, "mushroom_block_skin_brown");
        ADD_ICON(7, 15, "stem_bent");

        ADD_ICON(8, 0, "rail_normal");
        ADD_ICON(8, 1, "wool_colored_red");
        ADD_ICON(8, 2, "wool_colored_pink");
        ADD_ICON(8, 3, "repeater_off");
        ADD_ICON(8, 4, "leaves_spruce");
        ADD_ICON(8, 5, "leaves_spruce_opaque");
        ADD_ICON(8, 6, "bed_feet_top");
        ADD_ICON(8, 7, "bed_head_top");
        ADD_ICON(8, 8, "melon_side");
        ADD_ICON(8, 9, "melon_top");
        ADD_ICON(8, 10, "cauldron_top");
        ADD_ICON(8, 11, "cauldron_inner");
        // ADD_ICON(8,		12,	"unused");
        ADD_ICON(8, 13, "mushroom_block_skin_stem");
        ADD_ICON(8, 14, "mushroom_block_inside");
        ADD_ICON(8, 15, "vine");

        ADD_ICON(9, 0, "lapis_block");
        ADD_ICON(9, 1, "wool_colored_green");
        ADD_ICON(9, 2, "wool_colored_lime");
        ADD_ICON(9, 3, "repeater_on");
        ADD_ICON(9, 4, "glass_pane_top");
        ADD_ICON(9, 5, "bed_feet_end");
        ADD_ICON(9, 6, "bed_feet_side");
        ADD_ICON(9, 7, "bed_head_side");
        ADD_ICON(9, 8, "bed_head_end");
        ADD_ICON(9, 9, "log_jungle");
        ADD_ICON(9, 10, "cauldron_side");
        ADD_ICON(9, 11, "cauldron_bottom");
        ADD_ICON(9, 12, "brewing_stand_base");
        ADD_ICON(9, 13, "brewing_stand");
        ADD_ICON(9, 14, "endframe_top");
        ADD_ICON(9, 15, "endframe_side");

        ADD_ICON(10, 0, "lapis_ore");
        ADD_ICON(10, 1, "wool_colored_brown");
        ADD_ICON(10, 2, "wool_colored_yellow");
        ADD_ICON(10, 3, "rail_golden");
        ADD_ICON(10, 4, "redstone_dust_cross");
        ADD_ICON(10, 5, "redstone_dust_line");
        ADD_ICON(10, 6, "enchantment_top");
        ADD_ICON(10, 7, "dragon_egg");
        ADD_ICON(10, 8, "cocoa_2");
        ADD_ICON(10, 9, "cocoa_1");
        ADD_ICON(10, 10, "cocoa_0");
        ADD_ICON(10, 11, "emerald_ore");
        ADD_ICON(10, 12, "trip_wire_source");
        ADD_ICON(10, 13, "trip_wire");
        ADD_ICON(10, 14, "endframe_eye");
        ADD_ICON(10, 15, "end_stone");

        ADD_ICON(11, 0, "sandstone_top");
        ADD_ICON(11, 1, "wool_colored_blue");
        ADD_ICON(11, 2, "wool_colored_light_blue");
        ADD_ICON(11, 3, "rail_golden_powered");
        ADD_ICON(11, 4, "redstone_dust_cross_overlay");
        ADD_ICON(11, 5, "redstone_dust_line_overlay");
        ADD_ICON(11, 6, "enchantment_side");
        ADD_ICON(11, 7, "enchantment_bottom");
        ADD_ICON(11, 8, "command_block");
        ADD_ICON(11, 9, "itemframe_back");
        ADD_ICON(11, 10, "flower_pot");
        ADD_ICON(11, 11, "comparator_off");
        ADD_ICON(11, 12, "comparator_on");
        ADD_ICON(11, 13, "rail_activator");
        ADD_ICON(11, 14, "rail_activator_powered");
        ADD_ICON(11, 15, "quartz_ore");

        ADD_ICON(12, 0, "sandstone_side");
        ADD_ICON(12, 1, "wool_colored_purple");
        ADD_ICON(12, 2, "wool_colored_magenta");
        ADD_ICON(12, 3, "detectorRail");
        ADD_ICON(12, 4, "leaves_jungle");
        ADD_ICON(12, 5, "leaves_jungle_opaque");
        ADD_ICON(12, 6, "planks_spruce");
        ADD_ICON(12, 7, "planks_jungle");
        ADD_ICON(12, 8, "carrots_stage_0");
        ADD_ICON(12, 9, "carrots_stage_1");
        ADD_ICON(12, 10, "carrots_stage_2");
        ADD_ICON(12, 11, "carrots_stage_3");
        // ADD_ICON(12,	12,	"unused");
        ADD_ICON(12, 13, "water");
        ADD_ICON_SIZE(12, 14, "water_flow", 2, 2);

        ADD_ICON(13, 0, "sandstone_bottom");
        ADD_ICON(13, 1, "wool_colored_cyan");
        ADD_ICON(13, 2, "wool_colored_orange");
        ADD_ICON(13, 3, "redstoneLight");
        ADD_ICON(13, 4, "redstoneLight_lit");
        ADD_ICON(13, 5, "stonebrick_carved");
        ADD_ICON(13, 6, "planks_birch");
        ADD_ICON(13, 7, "anvil_base");
        ADD_ICON(13, 8, "anvil_top_damaged_1");
        ADD_ICON(13, 9, "quartz_block_chiseled_top");
        ADD_ICON(13, 10, "quartz_block_lines_top");
        ADD_ICON(13, 11, "quartz_block_top");
        ADD_ICON(13, 12, "hopper_outside");
        ADD_ICON(13, 13, "detectorRail_on");

        ADD_ICON(14, 0, "nether_brick");
        ADD_ICON(14, 1, "wool_colored_silver");
        ADD_ICON(14, 2, "nether_wart_stage_0");
        ADD_ICON(14, 3, "nether_wart_stage_1");
        ADD_ICON(14, 4, "nether_wart_stage_2");
        ADD_ICON(14, 5, "sandstone_carved");
        ADD_ICON(14, 6, "sandstone_smooth");
        ADD_ICON(14, 7, "anvil_top");
        ADD_ICON(14, 8, "anvil_top_damaged_2");
        ADD_ICON(14, 9, "quartz_block_chiseled");
        ADD_ICON(14, 10, "quartz_block_lines");
        ADD_ICON(14, 11, "quartz_block_side");
        ADD_ICON(14, 12, "hopper_inside");
        ADD_ICON(14, 13, "lava");
        ADD_ICON_SIZE(14, 14, "lava_flow", 2, 2);

        ADD_ICON(15, 0, "destroy_0");
        ADD_ICON(15, 1, "destroy_1");
        ADD_ICON(15, 2, "destroy_2");
        ADD_ICON(15, 3, "destroy_3");
        ADD_ICON(15, 4, "destroy_4");
        ADD_ICON(15, 5, "destroy_5");
        ADD_ICON(15, 6, "destroy_6");
        ADD_ICON(15, 7, "destroy_7");
        ADD_ICON(15, 8, "destroy_8");
        ADD_ICON(15, 9, "destroy_9");
        ADD_ICON(15, 10, "hay_block_side");
        ADD_ICON(15, 11, "quartz_block_bottom");
        ADD_ICON(15, 12, "hopper_top");
        ADD_ICON(15, 13, "hay_block_top");

        ADD_ICON(16, 0, "coal_block");
        ADD_ICON(16, 1, "hardened_clay");
        ADD_ICON(16, 2, "noteblock");
        // ADD_ICON(16,	3,	"unused");
        // ADD_ICON(16,	4,	"unused");
        // ADD_ICON(16,	5,	"unused");
        // ADD_ICON(16,	6,	"unused");
        // ADD_ICON(16,	7,	"unused");
        // ADD_ICON(16,	8,	"unused");
        ADD_ICON(16, 9, "potatoes_stage_0");
        ADD_ICON(16, 10, "potatoes_stage_1");
        ADD_ICON(16, 11, "potatoes_stage_2");
        ADD_ICON(16, 12, "potatoes_stage_3");
        ADD_ICON(16, 13, "log_spruce_top");
        ADD_ICON(16, 14, "log_jungle_top");
        ADD_ICON(16, 15, "log_birch_top");

        ADD_ICON(17, 0, "hardened_clay_stained_black");
        ADD_ICON(17, 1, "hardened_clay_stained_blue");
        ADD_ICON(17, 2, "hardened_clay_stained_brown");
        ADD_ICON(17, 3, "hardened_clay_stained_cyan");
        ADD_ICON(17, 4, "hardened_clay_stained_gray");
        ADD_ICON(17, 5, "hardened_clay_stained_green");
        ADD_ICON(17, 6, "hardened_clay_stained_light_blue");
        ADD_ICON(17, 7, "hardened_clay_stained_lime");
        ADD_ICON(17, 8, "hardened_clay_stained_magenta");
        ADD_ICON(17, 9, "hardened_clay_stained_orange");
        ADD_ICON(17, 10, "hardened_clay_stained_pink");
        ADD_ICON(17, 11, "hardened_clay_stained_purple");
        ADD_ICON(17, 12, "hardened_clay_stained_red");
        ADD_ICON(17, 13, "hardened_clay_stained_silver");
        ADD_ICON(17, 14, "hardened_clay_stained_white");
        ADD_ICON(17, 15, "hardened_clay_stained_yellow");

        ADD_ICON(18, 0, "glass_black");
        ADD_ICON(18, 1, "glass_blue");
        ADD_ICON(18, 2, "glass_brown");
        ADD_ICON(18, 3, "glass_cyan");
        ADD_ICON(18, 4, "glass_gray");
        ADD_ICON(18, 5, "glass_green");
        ADD_ICON(18, 6, "glass_light_blue");
        ADD_ICON(18, 7, "glass_lime");
        ADD_ICON(18, 8, "glass_magenta");
        ADD_ICON(18, 9, "glass_orange");
        ADD_ICON(18, 10, "glass_pink");
        ADD_ICON(18, 11, "glass_purple");
        ADD_ICON(18, 12, "glass_red");
        ADD_ICON(18, 13, "glass_silver");
        ADD_ICON(18, 14, "glass_white");
        ADD_ICON(18, 15, "glass_yellow");

        ADD_ICON(19, 0, "glass_pane_top_black");
        ADD_ICON(19, 1, "glass_pane_top_blue");
        ADD_ICON(19, 2, "glass_pane_top_brown");
        ADD_ICON(19, 3, "glass_pane_top_cyan");
        ADD_ICON(19, 4, "glass_pane_top_gray");
        ADD_ICON(19, 5, "glass_pane_top_green");
        ADD_ICON(19, 6, "glass_pane_top_light_blue");
        ADD_ICON(19, 7, "glass_pane_top_lime");
        ADD_ICON(19, 8, "glass_pane_top_magenta");
        ADD_ICON(19, 9, "glass_pane_top_orange");
        ADD_ICON(19, 10, "glass_pane_top_pink");
        ADD_ICON(19, 11, "glass_pane_top_purple");
        ADD_ICON(19, 12, "glass_pane_top_red");
        ADD_ICON(19, 13, "glass_pane_top_silver");
        ADD_ICON(19, 14, "glass_pane_top_white");
        ADD_ICON(19, 15, "glass_pane_top_yellow");
    }
}
