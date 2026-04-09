#include "Textures.h"

#include <assert.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "HttpTexture.h"
#include "java/Buffer.h"
#include "java/ByteBuffer.h"
#include "minecraft/IGameServices.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/MemoryTracker.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/renderer/MemTexture.h"
#include "minecraft/client/renderer/MemTextureProcessor.h"
#include "minecraft/client/renderer/MobSkinMemTextureProcessor.h"
#include "minecraft/client/renderer/texture/PreStitchedTextureMap.h"
#include "minecraft/client/renderer/texture/Texture.h"
#include "minecraft/client/renderer/texture/TextureAtlas.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/entity/item/ItemEntity.h"
#include "minecraft/world/item/ItemInstance.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

// Linux/PC port: disable mipmapping globally so textures are always sampled
// from the full-resolution level 0 with GL_NEAREST, giving pixel-crisp
// Minecraft blocks at all distances. Mipmapping causes glGenerateMipmap() to
// fire (which resets the min-filter to GL_NEAREST_MIPMAP_LINEAR on many
// Mesa/Nvidia drivers) and the per-level crispBlend loop is both wasteful and
// still causes visible blurring.
bool Textures::MIPMAP = false;
IPlatformRenderer::eTextureFormat Textures::TEXTURE_FORMAT =
    IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw;

int Textures::preLoadedIdx[TN_COUNT];
const char* Textures::preLoaded[TN_COUNT] = {
    "%blur%misc/pumpkinblur",
    "%clamp%misc/shadow",
    "art/kz",
    "environment/clouds",
    "environment/rain",
    "environment/snow",
    "gui/gui",
    "gui/icons",
    "item/arrows",
    "item/boat",
    "item/cart",
    "item/sign",
    "misc/mapbg",
    "misc/mapicons",
    "misc/water",
    "misc/footprint",
    "mob/saddle",
    "mob/sheep_fur",
    "mob/spider_eyes",
    "particles",
    "mob/chicken",
    "mob/cow",
    "mob/pig",
    "mob/sheep",
    "mob/squid",
    "mob/wolf",
    "mob/wolf_tame",
    "mob/wolf_angry",
    "mob/creeper",
    "mob/ghast",
    "mob/ghast_fire",
    "mob/zombie",
    "mob/pigzombie",
    "mob/skeleton",
    "mob/slime",
    "mob/spider",
    "mob/char",
    "mob/char1",
    "mob/char2",
    "mob/char3",
    "mob/char4",
    "mob/char5",
    "mob/char6",
    "mob/char7",
    "terrain/moon",
    "terrain/sun",
    "armor/power",

    // 1.8.2
    "mob/cavespider",
    "mob/enderman",
    "mob/silverfish",
    "mob/enderman_eyes",
    "misc/explosion",
    "item/xporb",
    "item/chest",
    "item/largechest",

    // 1.3.2
    "item/enderchest",

    // 1.0.1
    "mob/redcow",
    "mob/snowman",
    "mob/enderdragon/ender",
    "mob/fire",
    "mob/lava",
    "mob/villager/villager",
    "mob/villager/farmer",
    "mob/villager/librarian",
    "mob/villager/priest",
    "mob/villager/smith",
    "mob/villager/butcher",
    "mob/enderdragon/crystal",
    "mob/enderdragon/shuffle",
    "mob/enderdragon/beam",
    "mob/enderdragon/ender_eyes",
    "%blur%misc/glint",
    "item/book",
    "misc/tunnel",
    "misc/particlefield",
    "terrain/moon_phases",

    // 1.2.3
    "mob/ozelot",
    "mob/cat_black",
    "mob/cat_red",
    "mob/cat_siamese",
    "mob/villager_golem",
    "mob/skeleton_wither",

    // TU 14
    "mob/wolf_collar",
    "mob/zombie_villager",

    // 1.6.4
    "item/lead_knot",

    "misc/beacon_beam",

    "mob/bat",

    "mob/horse/donkey",
    "mob/horse/horse_black",
    "mob/horse/horse_brown",
    "mob/horse/horse_chestnut",
    "mob/horse/horse_creamy",
    "mob/horse/horse_darkbrown",
    "mob/horse/horse_gray",
    "mob/horse/horse_markings_blackdots",
    "mob/horse/horse_markings_white",
    "mob/horse/horse_markings_whitedots",
    "mob/horse/horse_markings_whitefield",
    "mob/horse/horse_skeleton",
    "mob/horse/horse_white",
    "mob/horse/horse_zombie",
    "mob/horse/mule",

    "mob/horse/armor/horse_armor_diamond",
    "mob/horse/armor/horse_armor_gold",
    "mob/horse/armor/horse_armor_iron",

    "mob/witch",

    "mob/wither/wither",
    "mob/wither/wither_armor",
    "mob/wither/wither_invulnerable",

    "item/trapped",
    "item/trapped_double",

// 4jcraft: java UI specific
#ifdef ENABLE_JAVA_GUIS
    "%blur%/misc/vignette",
    "/achievement/bg",
    "gui/background",
    "gui/inventory",
    "gui/container",
    "gui/crafting",
    "gui/furnace",
    "gui/creative_inventory/tabs",
    "gui/creative_inventory/tab_items",
    "gui/creative_inventory/tab_inventory",
    "gui/creative_inventory/tab_item_search",
    "title/mclogo",
    "gui/horse",
    "gui/anvil",
    "gui/trap",
    "gui/beacon",
    "gui/hopper",
    "gui/enchant",
    "gui/villager",
    "gui/brewing_stand",
    "title/bg/panorama",
    "title/bg/panorama0",
    "title/bg/panorama1",
    "title/bg/panorama2",
    "title/bg/panorama3",
    "title/bg/panorama4",
    "title/bg/panorama5",
#endif
// "item/christmas",
// "item/christmas_double",

#if defined(_LARGE_WORLDS)
    "misc/additionalmapicons",
#endif

    "font/Default",
    "font/alternate",

    // skin packs
    /*	"/SP1",
            "/SP2",
            "/SP3",
            "/SPF",

            // themes
            "/ThSt",
            "/ThIr",
            "/ThGo",
            "/ThDi",

            // gamerpics
            "/GPAn",
            "/GPCo",
            "/GPEn",
            "/GPFo",
            "/GPTo",
            "/GPBA",
            "/GPFa",
            "/GPME",
            "/GPMF",
            "/GPMM",
            "/GPSE",

            // avatar items

            "/AH_0006",
            "/AH_0003",
            "/AH_0007",
            "/AH_0005",
            "/AH_0004",
            "/AH_0001",
            "/AH_0002",
            "/AT_0001",
            "/AT_0002",
            "/AT_0003",
            "/AT_0004",
            "/AT_0005",
            "/AT_0006",
            "/AT_0007",
            "/AT_0008",
            "/AT_0009",
            "/AT_0010",
            "/AT_0011",
            "/AT_0012",
            "/AP_0001",
            "/AP_0002",
            "/AP_0003",
            "/AP_0004",
            "/AP_0005",
            "/AP_0006",
            "/AP_0007",
            "/AP_0009",
            "/AP_0010",
            "/AP_0011",
            "/AP_0012",
            "/AP_0013",
            "/AP_0014",
            "/AP_0015",
            "/AP_0016",
            "/AP_0017",
            "/AP_0018",
            "/AA_0001",
            "/AT_0013",
            "/AT_0014",
            "/AT_0015",
            "/AT_0016",
            "/AT_0017",
            "/AT_0018",
            "/AP_0019",
            "/AP_0020",
            "/AP_0021",
            "/AP_0022",
            "/AP_0023",
            "/AH_0008",
            "/AH_0009",*/

    "gui/items",
    "terrain",
};

Textures::Textures(TexturePackRepository* skins, Options* options) {
    //    pixels = MemoryTracker::createIntBuffer(2048 * 2048);	// 4J removed -
    //    now just creating this buffer when we need it
    missingNo = new BufferedImage(16, 16, BufferedImage::TYPE_INT_ARGB);

    this->skins = skins;
    this->options = options;

    /* 4J - TODO, maybe...
    Graphics g = missingNo.getGraphics();
    g.setColor(Color.WHITE);
    g.fillRect(0, 0, 64, 64);
    g.setColor(Color.BLACK);
    int y = 10;
    int i = 0;
    while (y < 64) {
            String text = (i++ % 2 == 0) ? "missing" : "texture";
            g.drawString(text, 1, y);
            y += g.getFont().getSize();
            if (i % 2 == 0) y += 5;
    }

g.dispose();
    */

    // 4J Stu - Changed these to our PreStitchedTextureMap from TextureMap
    terrain = new PreStitchedTextureMap(Icon::TYPE_TERRAIN, "terrain",
                                        "textures/blocks/", missingNo, true);
    items = new PreStitchedTextureMap(Icon::TYPE_ITEM, "items",
                                      "textures/items/", missingNo, true);

    // 4J - added - preload a set of commonly used textures that can then be
    // referenced directly be an enumerated type rather by string
    loadIndexedTextures();
}

void Textures::loadIndexedTextures() {
    // 4J - added - preload a set of commonly used textures that can then be
    // referenced directly be an enumerated type rather by string
    for (int i = 0; i < TN_COUNT - 2; i++) {
        preLoadedIdx[i] =
            loadTexture((TEXTURE_NAME)i, std::string(preLoaded[i]) + ".png");
    }
}

std::vector<int> Textures::loadTexturePixels(TEXTURE_NAME texId,
                                             const std::string& resourceName) {
    TexturePack* skin = skins->getSelected();

    {
        std::vector<int> id = pixelsMap[resourceName];
        // 4J - if resourceName isn't in the map, it should add an element and
        // as that will use the default constructor, its vector will be empty
        if (!id.empty()) return id;
    }

    // 4J - removed try/catch
    //    try {
    std::vector<int> res;
    // string in = skin->getResource(resourceName);
    if (false)  // 4J - removed - was ( in == nullptr)
    {
        res = loadTexturePixels(missingNo);
    } else {
        BufferedImage* bufImage = readImage(texId, resourceName);  // in);
        res = loadTexturePixels(bufImage);
        delete bufImage;
    }

    pixelsMap[resourceName] = res;
    return res;
    /*
    }
            catch (IOException e) {
            e.printStackTrace();
            int[] res = loadTexturePixels(missingNo);
            pixelsMap.put(resourceName, res);
            return res;
        }
            */
}

std::vector<int> Textures::loadTexturePixels(BufferedImage* img) {
    int w = img->getWidth();
    int h = img->getHeight();
    std::vector<int> pixels(w * h);
    return loadTexturePixels(img, pixels);
}

std::vector<int> Textures::loadTexturePixels(BufferedImage* img,
                                             std::vector<int>& pixels) {
    int w = img->getWidth();
    int h = img->getHeight();
    img->getRGB(0, 0, w, h, pixels, 0, w);
    return pixels;
}

int Textures::loadTexture(int idx) {
    if (idx == -1) {
        return 0;
    } else {
        if (idx == TN_TERRAIN) {
            terrain->getStitchedTexture()->bind(0);
            return terrain->getStitchedTexture()->getGlId();
        }
        if (idx == TN_GUI_ITEMS) {
            items->getStitchedTexture()->bind(0);
            return items->getStitchedTexture()->getGlId();
        }
        return preLoadedIdx[idx];
    }
}

// 4J added - textures default to standard 32-bit RGBA format, but where we can,
// use an 8-bit format. There's 3 different varieties of these currently in the
// renderer that map the single 8-bit channel to RGBA differently.
void Textures::setTextureFormat(const std::string& resourceName) {
    // 4J Stu - These texture formats are not currently in the render header
    { TEXTURE_FORMAT = IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw; }
}

void Textures::bindTexture(const std::string& resourceName) {
    bind(loadTexture(TN_COUNT, resourceName));
}

// 4J Added
void Textures::bindTexture(ResourceLocation* resource) {
    if (resource->isPreloaded()) {
        bind(loadTexture(resource->getTexture()));
    } else {
        bind(loadTexture(TN_COUNT, resource->getPath()));
    }
}

// 4jcraft: brought over from smartcmd/MinecraftConsoles in TU19 merge
void Textures::bindTextureLayers(ResourceLocation* resource) {
    assert(resource->isPreloaded());

    // Hack: 4JLibs on Windows does not currently reproduce Minecraft's layered
    // horse texture path reliably. Merge the layers on the CPU and bind the
    // cached result as a normal single texture instead.
    std::string cacheKey = "%layered%";
    int layers = resource->getTextureCount();
    for (int i = 0; i < layers; i++) {
        cacheKey += std::to_string(resource->getTexture(i));
        cacheKey += "/";
    }

    int id = -1;
    bool inMap = (idMap.find(cacheKey) != idMap.end());
    if (inMap) {
        id = idMap[cacheKey];
    } else {
        // Cache by layer signature so the merge cost is only paid once per
        // horse texture combination.
        std::vector<int> mergedPixels;
        int mergedWidth = 0;
        int mergedHeight = 0;
        bool hasMergedPixels = false;

        for (int i = 0; i < layers; i++) {
            TEXTURE_NAME textureName = resource->getTexture(i);
            if (textureName == static_cast<_TEXTURE_NAME>(-1)) {
                continue;
            }

            std::string resourceName =
                std::string(preLoaded[textureName]) + ".png";
            BufferedImage* image = readImage(textureName, resourceName);
            if (image == nullptr) {
                continue;
            }

            int width = image->getWidth();
            int height = image->getHeight();
            std::vector<int> layerPixels = loadTexturePixels(image);
            delete image;

            if (!hasMergedPixels) {
                mergedWidth = width;
                mergedHeight = height;
                mergedPixels = std::vector<int>(width * height);
                memcpy(mergedPixels.data(), layerPixels.data(),
                       width * height * sizeof(int));
                hasMergedPixels = true;
            } else if (width == mergedWidth && height == mergedHeight) {
                for (int p = 0; p < width * height; p++) {
                    int dst = mergedPixels[p];
                    int src = layerPixels[p];

                    float srcAlpha = ((src >> 24) & 0xff) / 255.0f;
                    if (srcAlpha <= 0.0f) {
                        continue;
                    }

                    float dstAlpha = ((dst >> 24) & 0xff) / 255.0f;
                    float outAlpha = srcAlpha + dstAlpha * (1.0f - srcAlpha);
                    if (outAlpha <= 0.0f) {
                        mergedPixels[p] = 0;
                        continue;
                    }

                    float srcFactor = srcAlpha / outAlpha;
                    float dstFactor = (dstAlpha * (1.0f - srcAlpha)) / outAlpha;

                    int outA = static_cast<int>(outAlpha * 255.0f + 0.5f);
                    int outR = static_cast<int>(
                        (((src >> 16) & 0xff) * srcFactor) +
                        (((dst >> 16) & 0xff) * dstFactor) + 0.5f);
                    int outG = static_cast<int>(
                        (((src >> 8) & 0xff) * srcFactor) +
                        (((dst >> 8) & 0xff) * dstFactor) + 0.5f);
                    int outB =
                        static_cast<int>(((src & 0xff) * srcFactor) +
                                         ((dst & 0xff) * dstFactor) + 0.5f);
                    mergedPixels[p] =
                        (outA << 24) | (outR << 16) | (outG << 8) | outB;
                }
            }
        }

        if (hasMergedPixels) {
            BufferedImage* mergedImage = new BufferedImage(
                mergedWidth, mergedHeight, BufferedImage::TYPE_INT_ARGB);
            memcpy(mergedImage->getData(), mergedPixels.data(),
                   mergedWidth * mergedHeight * sizeof(int));
            id = getTexture(mergedImage,
                            IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw, false);
        } else {
            id = 0;
        }

        idMap[cacheKey] = id;
    }

    PlatformRenderer.TextureBind(id);
}

void Textures::bind(int id) {
    // 4jcraft: Classic GUI code still performs some raw glBindTexture calls, so
    // this path must always rebind rather than trusting lastBoundId to be in
    // sync.
    // TODO(4jcraft): Long term, route all texture binds through one
    // synchronized path or invalidate lastBoundId at every raw glBindTexture
    // call so this can safely use cached binds again without breaking font/UI
    // rendering. if (id != lastBoundId)
    {
        if (id < 0) return;
        glBindTexture(GL_TEXTURE_2D, id);
        // lastBoundId = id;
    }
}

ResourceLocation* Textures::getTextureLocation(std::shared_ptr<Entity> entity) {
    std::shared_ptr<ItemEntity> item =
        std::dynamic_pointer_cast<ItemEntity>(entity);
    int iconType = item->getItem()->getIconType();
    return getTextureLocation(iconType);
}

ResourceLocation* Textures::getTextureLocation(int iconType) {
    switch (iconType) {
        case Icon::TYPE_TERRAIN:
            return &TextureAtlas::LOCATION_BLOCKS;
        case Icon::TYPE_ITEM:
            return &TextureAtlas::LOCATION_ITEMS;
    }

    return &TextureAtlas::LOCATION_ITEMS;
}

void Textures::clearLastBoundId() { lastBoundId = -1; }

int Textures::loadTexture(TEXTURE_NAME texId, const std::string& resourceName) {
    // 	char buf[256];
    // 	strncpy(buf, resourceName.c_str(), 256);
    // 	printf("Textures::loadTexture name - %s\n",buf);

    // if (resourceName.compare("/terrain.png") == 0)
    //{
    //	terrain->getStitchedTexture()->bind(0);
    //	return terrain->getStitchedTexture()->getGlId();
    // }
    // if (resourceName.compare("/gui/items.png") == 0)
    //{
    //	items->getStitchedTexture()->bind(0);
    //	return items->getStitchedTexture()->getGlId();
    // }

    // If the texture is not present in the idMap, load it, otherwise return its
    // id

    {
        bool inMap = (idMap.find(resourceName) != idMap.end());
        int id = idMap[resourceName];
        if (inMap) return id;
    }

    std::string pathName = resourceName;

    // 4J - added special cases to avoid mipmapping on clouds & shadows
    if ((resourceName == "environment/clouds.png") ||
        (resourceName == "%clamp%misc/shadow.png") ||
        (resourceName == "%blur%misc/pumpkinblur.png") ||
        (resourceName == "%clamp%misc/shadow.png") ||
        (resourceName == "gui/icons.png") || (resourceName == "gui/gui.png") ||
        (resourceName == "misc/footprint.png")) {
        MIPMAP = false;
    }
    setTextureFormat(resourceName);

    // 4J - removed try/catch
    //    try {
    int id = MemoryTracker::genTextures();

    std::string prefix = "%blur%";
    bool blur = resourceName.substr(0, prefix.size()).compare(prefix) ==
                0;  // resourceName.startsWith("%blur%");
    if (blur) pathName = resourceName.substr(6);

    prefix = "%clamp%";
    bool clamp = resourceName.substr(0, prefix.size()).compare(prefix) ==
                 0;  // resourceName.startsWith("%clamp%");
    if (clamp) pathName = resourceName.substr(7);

    // string in = skins->getSelected()->getResource(pathName);
    if (false)  // 4J - removed was ( in == nullptr)
    {
        loadTexture(missingNo, id, blur, clamp);
    } else {
        // 4J Stu - Get resource above just returns the name for texture packs
        BufferedImage* bufImage = readImage(texId, pathName);  // in);
        loadTexture(bufImage, id, blur, clamp);
        delete bufImage;
    }

    idMap[resourceName] = id;
    MIPMAP = true;  // 4J added
    TEXTURE_FORMAT = IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw;
    return id;
    /*
} catch (IOException e) {
e.printStackTrace();
MemoryTracker.genTextures(ib);
int id = ib.get(0);
loadTexture(missingNo, id);
idMap.put(resourceName, id);
return id;
}
*/
}

int Textures::getTexture(BufferedImage* img,
                         IPlatformRenderer::eTextureFormat format,
                         bool mipmap) {
    int id = MemoryTracker::genTextures();
    TEXTURE_FORMAT = format;
    MIPMAP = mipmap;
    loadTexture(img, id);
    TEXTURE_FORMAT = IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw;
    MIPMAP = true;
    loadedImages[id] = img;
    return id;
}

void Textures::loadTexture(BufferedImage* img, int id) {
    //	printf("Textures::loadTexture BufferedImage %d\n",id);

    loadTexture(img, id, false, false);
}

void Textures::loadTexture(BufferedImage* img, int id, bool blur, bool clamp) {
    //	printf("Textures::loadTexture BufferedImage with blur and clamp
    //%d\n",id);
    int iMipLevels = 1;
    glBindTexture(GL_TEXTURE_2D, id);

    if (MIPMAP) {
        // Linux/PC port: force GL_NEAREST to avoid mip-level distance blurring
        // and keep Minecraft textures pixel-crisp at all distances.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        /*
         * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
         * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
         * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
         * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
         */
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    if (blur) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    if (clamp) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    int w = img->getWidth();
    int h = img->getHeight();

    std::vector<int> rawPixels(w * h);
    img->getRGB(0, 0, w, h, rawPixels, 0, w);

    if (options != nullptr && options->anaglyph3d) {
        rawPixels = anaglyph(rawPixels);
    }

    std::vector<uint8_t> newPixels(w * h * 4);
    for (unsigned int i = 0; i < rawPixels.size(); i++) {
        int a = (rawPixels[i] >> 24) & 0xff;
        int r = (rawPixels[i] >> 16) & 0xff;
        int g = (rawPixels[i] >> 8) & 0xff;
        int b = (rawPixels[i]) & 0xff;

        newPixels[i * 4 + 0] = (uint8_t)r;
        newPixels[i * 4 + 1] = (uint8_t)g;
        newPixels[i * 4 + 2] = (uint8_t)b;
        newPixels[i * 4 + 3] = (uint8_t)a;
    }
    // 4J - now creating a buffer of the size we require dynamically
    ByteBuffer* pixels = MemoryTracker::createByteBuffer(w * h * 4);
    pixels->clear();
    pixels->put(newPixels);
    pixels->position(0)->limit(newPixels.size());

    if (MIPMAP) {
        // 4J-PB - In the new XDK, the CreateTexture will fail if the number of
        // mipmaps is higher than the width & height passed in will allow!
        int iWidthMips = 1;
        int iHeightMips = 1;
        while ((8 << iWidthMips) < w) iWidthMips++;
        while ((8 << iHeightMips) < h) iHeightMips++;

        iMipLevels = (iWidthMips < iHeightMips) ? iWidthMips : iHeightMips;
        // PlatformRenderer.TextureSetTextureLevels(5);	// 4J added
        if (iMipLevels > 5) iMipLevels = 5;
        PlatformRenderer.TextureSetTextureLevels(iMipLevels);  // 4J added
    }
    PlatformRenderer.TextureData(w, h, pixels->getBuffer(), 0, TEXTURE_FORMAT);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL12.GL_BGRA,
    // GL12.GL_UNSIGNED_INT_8_8_8_8_REV, pixels);

    if (MIPMAP) {
        for (int level = 1; level < iMipLevels; level++) {
            int ow = w >> (level - 1);
            // int oh = h >> (level - 1);

            int ww = w >> level;
            int hh = h >> level;

            // 4J - added tempData so we aren't overwriting source data
            unsigned int* tempData = new unsigned int[ww * hh];
            // 4J - added - have we loaded mipmap data for this level? Use that
            // rather than generating if possible
            if (img->getData(level)) {
                memcpy(tempData, img->getData(level), ww * hh * 4);
                // Swap ARGB to RGBA
                for (int i = 0; i < ww * hh; i++) {
                    tempData[i] = (tempData[i] >> 24) | (tempData[i] << 8);
                }
            } else {
                for (int x = 0; x < ww; x++)
                    for (int y = 0; y < hh; y++) {
                        int c0 = pixels->getInt(
                            ((x * 2 + 0) + (y * 2 + 0) * ow) * 4);
                        int c1 = pixels->getInt(
                            ((x * 2 + 1) + (y * 2 + 0) * ow) * 4);
                        int c2 = pixels->getInt(
                            ((x * 2 + 1) + (y * 2 + 1) * ow) * 4);
                        int c3 = pixels->getInt(
                            ((x * 2 + 0) + (y * 2 + 1) * ow) * 4);
                        // 4J - convert our RGBA texels to ARGB that crispBlend
                        // is expecting 4jcraft, added uint cast to pervent
                        // shift of neg int
                        c0 =
                            ((c0 >> 8) & 0x00ffffff) | ((unsigned int)c0 << 24);
                        c1 =
                            ((c1 >> 8) & 0x00ffffff) | ((unsigned int)c1 << 24);
                        c2 =
                            ((c2 >> 8) & 0x00ffffff) | ((unsigned int)c2 << 24);
                        c3 =
                            ((c3 >> 8) & 0x00ffffff) | ((unsigned int)c3 << 24);
                        int col =
                            Texture::crispBlend(Texture::crispBlend(c0, c1),
                                                Texture::crispBlend(c2, c3));
                        // 4J - and back from ARGB -> RGBA
                        col = ((unsigned int)col << 8) | ((col >> 24) & 0xff);
                        tempData[x + y * ww] = col;
                    }
            }
            for (int x = 0; x < ww; x++)
                for (int y = 0; y < hh; y++) {
                    pixels->putInt((x + y * ww) * 4, tempData[x + y * ww]);
                }
            delete[] tempData;
            PlatformRenderer.TextureData(ww, hh, pixels->getBuffer(), level,
                                         TEXTURE_FORMAT);
        }
    }

    /*
     * if (MIPMAP) { GLU.gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h,
     * GL_RGBA, GL_UNSIGNED_BYTE, pixels); } else { }
     */
    delete pixels;  // 4J - now creating this dynamically
}

std::vector<int> Textures::anaglyph(std::vector<int>& rawPixels) {
    std::vector<int> result(rawPixels.size());
    for (unsigned int i = 0; i < rawPixels.size(); i++) {
        int a = (rawPixels[i] >> 24) & 0xff;
        int r = (rawPixels[i] >> 16) & 0xff;
        int g = (rawPixels[i] >> 8) & 0xff;
        int b = (rawPixels[i]) & 0xff;

        int rr = (r * 30 + g * 59 + b * 11) / 100;
        int gg = (r * 30 + g * 70) / (100);
        int bb = (r * 30 + b * 70) / (100);

        result[i] = a << 24 | rr << 16 | gg << 8 | bb;
    }

    return result;
}

void Textures::replaceTexture(std::vector<int>& rawPixels, int w, int h,
                              int id) {
    bind(id);

    // Removed in Java
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (options != nullptr && options->anaglyph3d) {
        rawPixels = anaglyph(rawPixels);
    }

    std::vector<uint8_t> newPixels(w * h * 4);
    for (unsigned int i = 0; i < rawPixels.size(); i++) {
        int a = (rawPixels[i] >> 24) & 0xff;
        int r = (rawPixels[i] >> 16) & 0xff;
        int g = (rawPixels[i] >> 8) & 0xff;
        int b = (rawPixels[i]) & 0xff;

        if (options != nullptr && options->anaglyph3d) {
            int rr = (r * 30 + g * 59 + b * 11) / 100;
            int gg = (r * 30 + g * 70) / (100);
            int bb = (r * 30 + b * 70) / (100);

            r = rr;
            g = gg;
            b = bb;
        }

        newPixels[i * 4 + 0] = (uint8_t)r;
        newPixels[i * 4 + 1] = (uint8_t)g;
        newPixels[i * 4 + 2] = (uint8_t)b;
        newPixels[i * 4 + 3] = (uint8_t)a;
    }
    ByteBuffer* pixels = MemoryTracker::createByteBuffer(
        w * h * 4);  // 4J - now creating dynamically
    pixels->put(newPixels);
    pixels->position(0)->limit(newPixels.size());

    // New
    // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL12.GL_BGRA,
    // GL12.GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    PlatformRenderer.TextureDataUpdate(0, 0, w, h, pixels->getBuffer(), 0);
    // Old
    // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
    // pixels);
    delete pixels;
}

// 4J - added. This is a more minimal version of replaceTexture that assumes the
// texture bytes are already in order, and so doesn't do any of the extra
// copying round that the original java version does
void Textures::replaceTextureDirect(const std::vector<int>& rawPixels, int w,
                                    int h, int id) {
    glBindTexture(GL_TEXTURE_2D, id);

    // Remove in Java
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    PlatformRenderer.TextureDataUpdate(0, 0, w, h,
                                       const_cast<int*>(rawPixels.data()), 0);
}

// 4J - added. This is a more minimal version of replaceTexture that assumes the
// texture bytes are already in order, and so doesn't do any of the extra
// copying round that the original java version does
void Textures::replaceTextureDirect(const std::vector<short>& rawPixels, int w,
                                    int h, int id) {
    glBindTexture(GL_TEXTURE_2D, id);

    // Remove in Java
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    PlatformRenderer.TextureDataUpdate(0, 0, w, h,
                                       const_cast<short*>(rawPixels.data()), 0);
}

void Textures::releaseTexture(int id) {
    loadedImages.erase(id);
    glDeleteTextures(id);
}

int Textures::loadHttpTexture(const std::string& url,
                              const std::string& backup) {
    HttpTexture* texture = httpTextures[url];
    if (texture != nullptr) {
        if (texture->loadedImage != nullptr && !texture->isLoaded) {
            if (texture->id < 0) {
                texture->id = getTexture(texture->loadedImage);
            } else {
                loadTexture(texture->loadedImage, texture->id);
            }
            texture->isLoaded = true;
        }
    }
    if (texture == nullptr || texture->id < 0) {
        if (backup.empty()) return -1;
        return loadTexture(TN_COUNT, backup);
    }
    return texture->id;
}

int Textures::loadHttpTexture(const std::string& url, int backup) {
    HttpTexture* texture = httpTextures[url];
    if (texture != nullptr) {
        if (texture->loadedImage != nullptr && !texture->isLoaded) {
            if (texture->id < 0) {
                texture->id = getTexture(texture->loadedImage);
            } else {
                loadTexture(texture->loadedImage, texture->id);
            }
            texture->isLoaded = true;
        }
    }
    if (texture == nullptr || texture->id < 0) {
        return loadTexture(backup);
    }
    return texture->id;
}

bool Textures::hasHttpTexture(const std::string& url) {
    return httpTextures.find(url) != httpTextures.end();
}

HttpTexture* Textures::addHttpTexture(const std::string& url,
                                      HttpTextureProcessor* processor) {
    HttpTexture* texture = httpTextures[url];
    if (texture == nullptr) {
        httpTextures[url] = new HttpTexture(url, processor);
    } else {
        texture->count++;
    }
    return texture;
}

void Textures::removeHttpTexture(const std::string& url) {
    HttpTexture* texture = httpTextures[url];
    if (texture != nullptr) {
        texture->count--;
        if (texture->count == 0) {
            if (texture->id >= 0) releaseTexture(texture->id);
            httpTextures.erase(url);
        }
    }
}

// 4J-PB - adding for texture in memory (from global title storage)
int Textures::loadMemTexture(const std::string& url,
                             const std::string& backup) {
    MemTexture* texture = nullptr;
    auto it = memTextures.find(url);
    if (it != memTextures.end()) {
        texture = (*it).second;
    }
    if (texture == nullptr && gameServices().isFileInMemoryTextures(url)) {
        // If we haven't loaded it yet, but we have the data for it then add it
        texture = addMemTexture(url, new MobSkinMemTextureProcessor());
    }
    if (texture != nullptr) {
        if (texture->loadedImage != nullptr && !texture->isLoaded) {
            // 4J - Disable mipmapping in general for skins & capes. Have seen
            // problems with edge-on polys for some eg mumbo jumbo
            if ((url.substr(0, 7) == "dlcskin") ||
                (url.substr(0, 7) == "dlccape")) {
                MIPMAP = false;
            }

            if (texture->id < 0) {
                texture->id = getTexture(
                    texture->loadedImage,
                    IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw, MIPMAP);
            } else {
                loadTexture(texture->loadedImage, texture->id);
            }
            texture->isLoaded = true;
            MIPMAP = true;
        }
    }
    if (texture == nullptr || texture->id < 0) {
        if (backup.empty()) return -1;
        return loadTexture(TN_COUNT, backup);
    }
    return texture->id;
}

int Textures::loadMemTexture(const std::string& url, int backup) {
    MemTexture* texture = nullptr;
    auto it = memTextures.find(url);
    if (it != memTextures.end()) {
        texture = (*it).second;
    }
    if (texture == nullptr && gameServices().isFileInMemoryTextures(url)) {
        // If we haven't loaded it yet, but we have the data for it then add it
        texture = addMemTexture(url, new MobSkinMemTextureProcessor());
    }
    if (texture != nullptr) {
        texture->ticksSinceLastUse = 0;
        if (texture->loadedImage != nullptr && !texture->isLoaded) {
            // 4J - Disable mipmapping in general for skins & capes. Have seen
            // problems with edge-on polys for some eg mumbo jumbo
            if ((url.substr(0, 7) == "dlcskin") ||
                (url.substr(0, 7) == "dlccape")) {
                MIPMAP = false;
            }
            if (texture->id < 0) {
                texture->id = getTexture(
                    texture->loadedImage,
                    IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw, MIPMAP);
            } else {
                loadTexture(texture->loadedImage, texture->id);
            }
            texture->isLoaded = true;
            MIPMAP = true;
        }
    }
    if (texture == nullptr || texture->id < 0) {
        return loadTexture(backup);
    }
    return texture->id;
}

MemTexture* Textures::addMemTexture(const std::string& name,
                                    MemTextureProcessor* processor) {
    MemTexture* texture = nullptr;
    auto it = memTextures.find(name);
    if (it != memTextures.end()) {
        texture = (*it).second;
    }
    if (texture == nullptr) {
        // can we find it in the app mem files?
        std::uint8_t* pbData = nullptr;
        unsigned int dwBytes = 0;
        gameServices().getMemFileDetails(name, &pbData, &dwBytes);

        if (dwBytes != 0) {
            texture = new MemTexture(name, pbData, dwBytes, processor);
            memTextures[name] = texture;
        } else {
            // 4J Stu - Make an entry for this anyway and we can populate it
            // later
            memTextures[name] = nullptr;
        }
    } else {
        texture->count++;
    }

    delete processor;

    return texture;
}

// MemTexture *Textures::getMemTexture(const string& url, MemTextureProcessor
// *processor)
// {
// 	MemTexture *texture = memTextures[url];
// 	if (texture != nullptr)
// 	{
// 		texture->count++;
// 	}
// 	return texture;
// }

void Textures::removeMemTexture(const std::string& url) {
    MemTexture* texture = nullptr;
    auto it = memTextures.find(url);
    if (it != memTextures.end()) {
        texture = (*it).second;

        // If it's nullptr then we should just remove the entry
        if (texture == nullptr) memTextures.erase(url);
    }
    if (texture != nullptr) {
        texture->count--;
        if (texture->count == 0) {
            if (texture->id >= 0) releaseTexture(texture->id);
            memTextures.erase(url);
            delete texture;
        }
    }
}

void Textures::tick(
    bool updateTextures,
    bool tickDynamics)  // 4J added updateTextures parameter & tickDynamics
{
    if (tickDynamics) {
        // 4J - added - if we aren't updating the final renderer textures, just
        // tick each of the dynamic textures instead. This is used so that in
        // frames were we have multiple ticks due to framerate compensation,
        // that we don't lock the renderer textures twice needlessly and force
        // the CPU to sync with the GPU.
        if (!updateTextures) {
            return;
        }

        // 4J - added - tell renderer that we're about to do a block of dynamic
        // texture updates, so we can unlock the resources after they are done
        // rather than a series of locks/unlocks
        // PlatformRenderer.TextureDynamicUpdateStart();
        terrain->cycleAnimationFrames();
        items->cycleAnimationFrames();
        // PlatformRenderer.TextureDynamicUpdateEnd();	// 4J added - see
        // comment above
    }

    // 4J - go over all the memory textures once per frame, and free any that
    // haven't been used for a while. Ones that are being used will have their
    // ticksSinceLastUse reset in Textures::loadMemTexture.
    for (auto it = memTextures.begin(); it != memTextures.end();) {
        MemTexture* tex = it->second;

        if (tex &&
            (++tex->ticksSinceLastUse > MemTexture::UNUSED_TICKS_TO_FREE)) {
            if (tex->id >= 0) releaseTexture(tex->id);
            delete tex;
            it = memTextures.erase(it);
        } else {
            it++;
        }
    }
}

void Textures::reloadAll() {
    TexturePack* skin = skins->getSelected();

    for (int i = 0; i < TN_COUNT - 2; i++) {
        releaseTexture(preLoadedIdx[i]);
    }

    idMap.clear();
    loadedImages.clear();

    loadIndexedTextures();

    pixelsMap.clear();
    // 4J Stu - These are not used any more
    // WaterColor::init(loadTexturePixels("misc/watercolor.png"));
    // GrassColor::init(loadTexturePixels("misc/grasscolor.png"));
    // FoliageColor::init(loadTexturePixels("misc/foliagecolor.png"));

    stitch();

    skins->clearInvalidTexturePacks();

    // Recalculate fonts
    // Minecraft::GetInstance()->font->loadCharacterWidths();
    // Minecraft::GetInstance()->altFont->loadCharacterWidths();
}

void Textures::stitch() {
    terrain->stitch();
    items->stitch();
}

Icon* Textures::getMissingIcon(int type) {
    switch (type) {
        case Icon::TYPE_ITEM:
        default:
            return items->getMissingIcon();
        case Icon::TYPE_TERRAIN:
            return terrain->getMissingIcon();
    }
}

BufferedImage* Textures::readImage(
    TEXTURE_NAME texId, const std::string& name)  // 4J was InputStream *in
{
    BufferedImage* img = nullptr;
    // is this image one of the Title Update ones?
    bool isTu = IsTUImage(texId, name);
    std::string drive = "";

    if (!skins->isUsingDefaultSkin() &&
        skins->getSelected()->hasFile("res/" + name, false)) {
        drive = skins->getSelected()->getPath(isTu);
        img = skins->getSelected()->getImageResource(
            name, false, isTu,
            drive);  // new BufferedImage(name,false,isTu,drive);
    } else {
        { drive = skins->getDefault()->getPath(isTu); }

        if (IsOriginalImage(texId, name) || isTu) {
            img = skins->getDefault()->getImageResource(
                name, false, isTu,
                drive);  // new BufferedImage(name,false,isTu,drive);
        } else {
            img = skins->getDefault()->getImageResource(
                "1_2_2/" + name, false, isTu,
                drive);  // new BufferedImage("/1_2_2" +
                         // name,false,isTu,drive);
        }
    }

    return img;
}

// Match the preload images from their enum to avoid a ton of string comparisons
TEXTURE_NAME TUImages[] = {
    TN_POWERED_CREEPER, TN_MOB_ENDERMAN_EYES, TN_MISC_EXPLOSION, TN_MOB_ZOMBIE,
    TN_MISC_FOOTSTEP, TN_MOB_RED_COW, TN_MOB_SNOWMAN, TN_MOB_ENDERDRAGON,
    TN_MOB_VILLAGER_VILLAGER, TN_MOB_VILLAGER_FARMER, TN_MOB_VILLAGER_LIBRARIAN,
    TN_MOB_VILLAGER_PRIEST, TN_MOB_VILLAGER_SMITH, TN_MOB_VILLAGER_BUTCHER,
    TN_MOB_ENDERDRAGON_ENDEREYES, TN__BLUR__MISC_GLINT, TN_ITEM_BOOK,
    TN_MISC_PARTICLEFIELD,

    // TU9
    TN_MISC_TUNNEL, TN_MOB_ENDERDRAGON_BEAM, TN_GUI_ITEMS, TN_TERRAIN,
    TN_MISC_MAPICONS,

    // TU12
    TN_MOB_WITHER_SKELETON,

    // TU14
    TN_TILE_ENDER_CHEST, TN_ART_KZ, TN_MOB_WOLF_TAME, TN_MOB_WOLF_COLLAR,
    TN_PARTICLES, TN_MOB_ZOMBIE_VILLAGER,

    TN_ITEM_LEASHKNOT,

    TN_MISC_BEACON_BEAM,

    TN_MOB_BAT,

    TN_MOB_DONKEY, TN_MOB_HORSE_BLACK, TN_MOB_HORSE_BROWN,
    TN_MOB_HORSE_CHESTNUT, TN_MOB_HORSE_CREAMY, TN_MOB_HORSE_DARKBROWN,
    TN_MOB_HORSE_GRAY, TN_MOB_HORSE_MARKINGS_BLACKDOTS,
    TN_MOB_HORSE_MARKINGS_WHITE, TN_MOB_HORSE_MARKINGS_WHITEDOTS,
    TN_MOB_HORSE_MARKINGS_WHITEFIELD, TN_MOB_HORSE_SKELETON, TN_MOB_HORSE_WHITE,
    TN_MOB_HORSE_ZOMBIE, TN_MOB_MULE, TN_MOB_HORSE_ARMOR_DIAMOND,
    TN_MOB_HORSE_ARMOR_GOLD, TN_MOB_HORSE_ARMOR_IRON,

    TN_MOB_WITCH,

    TN_MOB_WITHER, TN_MOB_WITHER_ARMOR, TN_MOB_WITHER_INVULNERABLE,

    TN_TILE_TRAP_CHEST, TN_TILE_LARGE_TRAP_CHEST,
// TN_TILE_XMAS_CHEST,
// TN_TILE_LARGE_XMAS_CHEST,

#if defined(_LARGE_WORLDS)
    TN_MISC_ADDITIONALMAPICONS,
#endif

    // TU17
    TN_DEFAULT_FONT,
    // TN_ALT_FONT, // Not in TU yet

    TN_COUNT  // Why is this here?
};

// This is for any TU textures that aren't part of our enum indexed preload set
const char* const TUImagePaths[] = {"font/Default", "font/Mojangles_7",
                                    "font/Mojangles_11",

                                    // TU12
                                    "armor/cloth_1.png", "armor/cloth_1_b.png",
                                    "armor/cloth_2.png", "armor/cloth_2_b.png",

                                    //

                                    nullptr};

bool Textures::IsTUImage(TEXTURE_NAME texId, const std::string& name) {
    int i = 0;
    if (texId < TN_COUNT) {
        while (TUImages[i] < TN_COUNT) {
            if (texId == TUImages[i]) {
                return true;
            }
            i++;
        }
    }
    i = 0;
    while (TUImagePaths[i]) {
        if (name.compare(TUImagePaths[i]) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

TEXTURE_NAME OriginalImages[] = {TN_MOB_CHAR,   TN_MOB_CHAR1, TN_MOB_CHAR2,
                                 TN_MOB_CHAR3,  TN_MOB_CHAR4, TN_MOB_CHAR5,
                                 TN_MOB_CHAR6,  TN_MOB_CHAR7,

                                 TN_MISC_MAPBG,

                                 TN_COUNT};

const char* const OriginalImagesPaths[] = {"misc/watercolor.png",

                                           nullptr};

bool Textures::IsOriginalImage(TEXTURE_NAME texId, const std::string& name) {
    int i = 0;
    if (texId < TN_COUNT) {
        while (OriginalImages[i] < TN_COUNT) {
            if (texId == OriginalImages[i]) {
                return true;
            }
            i++;
        }
    }
    i = 0;
    while (OriginalImagesPaths[i]) {
        if (name.compare(OriginalImagesPaths[i]) == 0) {
            return true;
        }
        i++;
    }
    return false;
}