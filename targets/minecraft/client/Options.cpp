#include "minecraft/util/Log.h"
#include "Options.h"

#include "KeyMapping.h"
#include "app/common/Audio/SoundEngine.h"
#include "util/StringHelpers.h"
#include "java/File.h"
#include "java/InputOutputStream/BufferedReader.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "java/InputOutputStream/FileInputStream.h"
#include "java/InputOutputStream/FileOutputStream.h"
#include "java/InputOutputStream/InputStreamReader.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/renderer/LevelRenderer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/locale/I18n.h"
#include "minecraft/locale/Language.h"
#include "platform/stubs.h"

// 4J - the Option sub-class used to be an java enumerated type, trying to
// emulate that functionality here
const Options::Option Options::Option::options[17] = {
    Options::Option("options.music", true, false),
    Options::Option("options.sound", true, false),
    Options::Option("options.invertMouse", false, true),
    Options::Option("options.sensitivity", true, false),
    Options::Option("options.renderDistance", false, false),
    Options::Option("options.viewBobbing", false, true),
    Options::Option("options.anaglyph", false, true),
    Options::Option("options.advancedOpengl", false, true),
    Options::Option("options.framerateLimit", false, false),
    Options::Option("options.difficulty", false, false),
    Options::Option("options.graphics", false, false),
    Options::Option("options.ao", false, true),
    Options::Option("options.guiScale", false, false),
    Options::Option("options.fov", true, false),
    Options::Option("options.gamma", true, false),
    Options::Option("options.renderClouds", false, true),
    Options::Option("options.particles", false, false),
};

const Options::Option* Options::Option::MUSIC = &Options::Option::options[0];
const Options::Option* Options::Option::SOUND = &Options::Option::options[1];
const Options::Option* Options::Option::INVERT_MOUSE =
    &Options::Option::options[2];
const Options::Option* Options::Option::SENSITIVITY =
    &Options::Option::options[3];
const Options::Option* Options::Option::RENDER_DISTANCE =
    &Options::Option::options[4];
const Options::Option* Options::Option::VIEW_BOBBING =
    &Options::Option::options[5];
const Options::Option* Options::Option::ANAGLYPH = &Options::Option::options[6];
const Options::Option* Options::Option::ADVANCED_OPENGL =
    &Options::Option::options[7];
const Options::Option* Options::Option::FRAMERATE_LIMIT =
    &Options::Option::options[8];
const Options::Option* Options::Option::DIFFICULTY =
    &Options::Option::options[9];
const Options::Option* Options::Option::GRAPHICS =
    &Options::Option::options[10];
const Options::Option* Options::Option::AMBIENT_OCCLUSION =
    &Options::Option::options[11];
const Options::Option* Options::Option::GUI_SCALE =
    &Options::Option::options[12];
const Options::Option* Options::Option::FOV = &Options::Option::options[13];
const Options::Option* Options::Option::GAMMA = &Options::Option::options[14];
const Options::Option* Options::Option::RENDER_CLOUDS =
    &Options::Option::options[15];
const Options::Option* Options::Option::PARTICLES =
    &Options::Option::options[16];

const Options::Option* Options::Option::getItem(int id) { return &options[id]; }

Options::Option::Option(const std::string& captionId, bool hasProgress,
                        bool isBoolean)
    : _isProgress(hasProgress), _isBoolean(isBoolean), captionId(captionId) {}

bool Options::Option::isProgress() const { return _isProgress; }

bool Options::Option::isBoolean() const { return _isBoolean; }

int Options::Option::getId() const { return (int)(this - options); }

std::string Options::Option::getCaptionId() const { return captionId; }

const std::string Options::RENDER_DISTANCE_NAMES[] = {
    "options.renderDistance.far", "options.renderDistance.normal",
    "options.renderDistance.short", "options.renderDistance.tiny"};
const std::string Options::DIFFICULTY_NAMES[] = {
    "options.difficulty.peaceful", "options.difficulty.easy",
    "options.difficulty.normal", "options.difficulty.hard"};
const std::string Options::GUI_SCALE[] = {
    "options.guiScale.auto", "options.guiScale.small",
    "options.guiScale.normal", "options.guiScale.large"};

#ifdef ENABLE_VSYNC
const std::string Options::FRAMERATE_LIMITS[] = {
    "performance.max", "performance.balanced", "performance.powersaver"};
#else
const std::string Options::FRAMERATE_LIMITS[] = {
    "performance.max", "performance.balanced", "performance.powersaver",
    "performance.unlimited"};
#endif

const std::string Options::PARTICLES[] = {"options.particles.all",
                                           "options.particles.decreased",
                                           "options.particles.minimal"};

// 4J added
void Options::init() {
    music = 1;
    sound = 1;
    sensitivity = 0.5f;
    invertYMouse = false;
    viewDistance = 0;
    bobView = true;
    anaglyph3d = false;
    advancedOpengl = false;

// 4JCRAFT V-Sync / VSync
#if defined(ENABLE_VSYNC)
    framerateLimit = 2;
#else
    framerateLimit = 3;
#endif
    fancyGraphics = true;
    ambientOcclusion = true;
    renderClouds = true;
    skin = "Default";

    keyUp = new KeyMapping("key.forward", Keyboard::KEY_W);
    keyLeft = new KeyMapping("key.left", Keyboard::KEY_A);
    keyDown = new KeyMapping("key.back", Keyboard::KEY_S);
    keyRight = new KeyMapping("key.right", Keyboard::KEY_D);
    keyJump = new KeyMapping("key.jump", Keyboard::KEY_SPACE);
    keyBuild = new KeyMapping("key.inventory", Keyboard::KEY_E);
    keyDrop = new KeyMapping("key.drop", Keyboard::KEY_Q);
    keyChat = new KeyMapping("key.chat", Keyboard::KEY_T);
    keySneak = new KeyMapping("key.sneak", Keyboard::KEY_LSHIFT);
    keyAttack = new KeyMapping("key.attack", -100 + 0);
    keyUse = new KeyMapping("key.use", -100 + 1);
    keyPlayerList = new KeyMapping("key.playerlist", Keyboard::KEY_TAB);
    keyPickItem = new KeyMapping("key.pickItem", -100 + 2);
    keyToggleFog = new KeyMapping("key.fog", Keyboard::KEY_F);

    keyMappings[0] = keyAttack;
    keyMappings[1] = keyUse;
    keyMappings[2] = keyUp;
    keyMappings[3] = keyLeft;
    keyMappings[4] = keyDown;
    keyMappings[5] = keyRight;
    keyMappings[6] = keyJump;
    keyMappings[7] = keySneak;
    keyMappings[8] = keyDrop;
    keyMappings[9] = keyBuild;
    keyMappings[10] = keyChat;
    keyMappings[11] = keyPlayerList;
    keyMappings[12] = keyPickItem;
    keyMappings[13] = keyToggleFog;

    minecraft = nullptr;
    // optionsFile = nullptr;

    difficulty = 2;
    hideGui = false;
    thirdPersonView = false;
    renderDebug = false;
    lastMpIp = "";

    isFlying = false;
    smoothCamera = false;
    fixedCamera = false;
    flySpeed = 1;
    cameraSpeed = 1;
    guiScale = 3;
    particles = 0;
    fov = 0;
    gamma = 0;
}

Options::Options(Minecraft* minecraft, File workingDirectory) {
    init();
    this->minecraft = minecraft;
    optionsFile = File(workingDirectory, "options.txt");
}

Options::Options() { init(); }

std::string Options::getKeyDescription(int i) {
    Language* language = Language::getInstance();
    return language->getElement(keyMappings[i]->name);
}

std::string Options::getKeyMessage(int i) {
    int key = keyMappings[i]->key;
    if (key < 0) {
        return I18n::get("key.mouseButton", key + 101);
    } else {
        return Keyboard::getKeyName(keyMappings[i]->key);
    }
}

void Options::setKey(int i, int key) {
    keyMappings[i]->key = key;
    save();
}

void Options::set(const Options::Option* item, float fVal) {
    if (item == Option::MUSIC) {
        music = fVal;
        minecraft->soundEngine->updateMusicVolume(fVal);
    }
    if (item == Option::SOUND) {
        sound = fVal;
        minecraft->soundEngine->updateSoundEffectVolume(fVal);
    }
    if (item == Option::SENSITIVITY) {
        sensitivity = fVal;
    }
    if (item == Option::FOV) {
        fov = fVal;
    }
    if (item == Option::GAMMA) {
        gamma = fVal;
    }
}

void Options::toggle(const Options::Option* option, int dir) {
    if (option == Option::INVERT_MOUSE) invertYMouse = !invertYMouse;
    if (option == Option::RENDER_DISTANCE)
        viewDistance = (viewDistance + dir) & 3;
    if (option == Option::GUI_SCALE) guiScale = (guiScale + dir) & 3;
    if (option == Option::PARTICLES) particles = (particles + dir) % 3;

    // 4J-PB - changing
    // 4jcraft: uncommented this so that the view bobbing option works
    if (option == Option::VIEW_BOBBING) bobView = !bobView;
    if (option == Option::RENDER_CLOUDS) renderClouds = !renderClouds;
    if (option == Option::ADVANCED_OPENGL) {
        advancedOpengl = !advancedOpengl;
        // 4jcraft: ensure level exists before applying
        if (minecraft->level) minecraft->levelRenderer->allChanged();
    }
    if (option == Option::ANAGLYPH) {
        anaglyph3d = !anaglyph3d;
        minecraft->textures->reloadAll();
    }
    if (option == Option::FRAMERATE_LIMIT)
#ifdef ENABLE_VSYNC
        framerateLimit = (framerateLimit + dir + 3) % 3;
#else
        framerateLimit = (framerateLimit + dir + 4) % 4;
#endif

    // 4J-PB - Change for Xbox
    // if (option ==  Option::DIFFICULTY) difficulty = (difficulty + dir) & 3;
    if (option == Option::DIFFICULTY) difficulty = (dir) & 3;

    Log::info("Option::DIFFICULTY = %d", difficulty);

    if (option == Option::GRAPHICS) {
        fancyGraphics = !fancyGraphics;
        // 4jcraft: ensure level exists before applying
        if (minecraft->level) minecraft->levelRenderer->allChanged();
    }
    if (option == Option::AMBIENT_OCCLUSION) {
        ambientOcclusion = !ambientOcclusion;
        // 4jcraft: ensure level exists before applying
        if (minecraft->level) minecraft->levelRenderer->allChanged();
    }

    // 4J-PB - don't do the file save on the xbox
    // save();
}

float Options::getProgressValue(const Options::Option* item) {
    if (item == Option::FOV) return fov;
    if (item == Option::GAMMA) return gamma;
    if (item == Option::MUSIC) return music;
    if (item == Option::SOUND) return sound;
    if (item == Option::SENSITIVITY) return sensitivity;
    return 0;
}

bool Options::getBooleanValue(const Options::Option* item) {
    // 4J - was a switch statement which we can't do with our Option:: pointer
    // types
    if (item == Option::INVERT_MOUSE) return invertYMouse;
    if (item == Option::VIEW_BOBBING) return bobView;
    if (item == Option::ANAGLYPH) return anaglyph3d;
    if (item == Option::ADVANCED_OPENGL) return advancedOpengl;
    if (item == Option::AMBIENT_OCCLUSION) return ambientOcclusion;
    if (item == Option::RENDER_CLOUDS) return renderClouds;
    return false;
}

std::string Options::getMessage(const Options::Option* item) {
    // 4J TODO, should these std::wstrings append rather than add?

    Language* language = Language::getInstance();
    std::string caption = language->getElement(item->getCaptionId()) + ": ";

    if (item->isProgress()) {
        float progressValue = getProgressValue(item);

        if (item == Option::SENSITIVITY) {
            if (progressValue == 0) {
                return caption +
                       language->getElement("options.sensitivity.min");
            }
            if (progressValue == 1) {
                return caption +
                       language->getElement("options.sensitivity.max");
            }
            return caption + toWString<int>((int)(progressValue * 200)) + "%";
        } else if (item == Option::FOV) {
            if (progressValue == 0) {
                return caption + language->getElement("options.fov.min");
            }
            if (progressValue == 1) {
                return caption + language->getElement("options.fov.max");
            }
            return caption + toWString<int>((int)(70 + progressValue * 40));
        } else if (item == Option::GAMMA) {
            if (progressValue == 0) {
                return caption + language->getElement("options.gamma.min");
            }
            if (progressValue == 1) {
                return caption + language->getElement("options.gamma.max");
            }
            return caption + "+" + toWString<int>((int)(progressValue * 100)) +
                   "%";
        } else {
            if (progressValue == 0) {
                return caption + language->getElement("options.off");
            }
            return caption + toWString<int>((int)(progressValue * 100)) + "%";
        }
    } else if (item->isBoolean()) {
        bool booleanValue = getBooleanValue(item);
        if (booleanValue) {
            return caption + language->getElement("options.on");
        }
        return caption + language->getElement("options.off");
    } else if (item == Option::RENDER_DISTANCE) {
        return caption +
               language->getElement(RENDER_DISTANCE_NAMES[viewDistance]);
    } else if (item == Option::DIFFICULTY) {
        return caption + language->getElement(DIFFICULTY_NAMES[difficulty]);
    } else if (item == Option::GUI_SCALE) {
        return caption + language->getElement(GUI_SCALE[guiScale]);
    } else if (item == Option::PARTICLES) {
        return caption + language->getElement(PARTICLES[particles]);
    } else if (item == Option::FRAMERATE_LIMIT) {
        return caption + I18n::get(FRAMERATE_LIMITS[framerateLimit]);
    } else if (item == Option::GRAPHICS) {
        if (fancyGraphics) {
            return caption + language->getElement("options.graphics.fancy");
        }
        return caption + language->getElement("options.graphics.fast");
    }

    return caption;
}

void Options::load() {
    // 4J - removed try/catch
    //    try {
    if (!optionsFile.exists()) return;
    // 4J - was new BufferedReader(new FileReader(optionsFile));
    BufferedReader* br = new BufferedReader(
        new InputStreamReader(new FileInputStream(optionsFile)));

    std::string line = "";
    while ((line = br->readLine()) !=
           "")  // 4J - was check against nullptr - do we need to distinguish
                 // between empty lines and a fail here?
    {
        // 4J - removed try/catch
        //            try {
        std::string cmds[2];
        int splitpos = (int)line.find(":");
        if (splitpos == std::string::npos) {
            cmds[0] = line;
            cmds[1] = "";
        } else {
            cmds[0] = line.substr(0, splitpos);
            cmds[1] = line.substr(splitpos, line.length() - splitpos);
        }

        if (cmds[0] == "music") music = readFloat(cmds[1]);
        if (cmds[0] == "sound") sound = readFloat(cmds[1]);
        if (cmds[0] == "mouseSensitivity") sensitivity = readFloat(cmds[1]);
        if (cmds[0] == "fov") fov = readFloat(cmds[1]);
        if (cmds[0] == "gamma") gamma = readFloat(cmds[1]);
        if (cmds[0] == "invertYMouse") invertYMouse = cmds[1] == "true";
        if (cmds[0] == "viewDistance")
            viewDistance = fromWString<int>(cmds[1]);
        if (cmds[0] == "guiScale") guiScale = fromWString<int>(cmds[1]);
        if (cmds[0] == "particles") particles = fromWString<int>(cmds[1]);
        if (cmds[0] == "bobView") bobView = cmds[1] == "true";
        if (cmds[0] == "anaglyph3d") anaglyph3d = cmds[1] == "true";
        if (cmds[0] == "advancedOpengl") advancedOpengl = cmds[1] == "true";
        if (cmds[0] == "fpsLimit") framerateLimit = fromWString<int>(cmds[1]);
        if (cmds[0] == "difficulty") difficulty = fromWString<int>(cmds[1]);
        if (cmds[0] == "fancyGraphics") fancyGraphics = cmds[1] == "true";
        if (cmds[0] == "ao") ambientOcclusion = cmds[1] == "true";
        if (cmds[0] == "clouds") renderClouds = cmds[1] == "true";
        if (cmds[0] == "skin") skin = cmds[1];
        if (cmds[0] == "lastServer") lastMpIp = cmds[1];

        for (int i = 0; i < keyMappings_length; i++) {
            if (cmds[0] == ("key_" + keyMappings[i]->name)) {
                keyMappings[i]->key = fromWString<int>(cmds[1]);
            }
        }
        //            } catch (Exception e) {
        //                System.out.println("Skipping bad option: " + line);
        //            }
    }
    // KeyMapping.resetMapping(); // 4J Not implemented
    br->close();
    //    } catch (Exception e) {
    //        System.out.println("Failed to load options");
    //        e.printStackTrace();
    //    }
}

float Options::readFloat(std::string string) {
    if (string == "true") return 1;
    if (string == "false") return 0;
    return fromWString<float>(string);
}

void Options::save() {
    // 4J - try/catch removed
    //    try {

    // 4J - original used a PrintWriter & FileWriter, but seems a bit much
    // implementing these just to do this
    FileOutputStream fos = FileOutputStream(optionsFile);
    DataOutputStream dos = DataOutputStream(&fos);
    //        PrintWriter pw = new PrintWriter(new FileWriter(optionsFile));

    dos.writeChars("music:" + toWString<float>(music) + "\n");
    dos.writeChars("sound:" + toWString<float>(sound) + "\n");
    dos.writeChars("invertYMouse:" +
                   std::string(invertYMouse ? "true" : "false") + "\n");
    dos.writeChars("mouseSensitivity:" + toWString<float>(sensitivity));
    dos.writeChars("fov:" + toWString<float>(fov));
    dos.writeChars("gamma:" + toWString<float>(gamma));
    dos.writeChars("viewDistance:" + toWString<int>(viewDistance));
    dos.writeChars("guiScale:" + toWString<int>(guiScale));
    dos.writeChars("particles:" + toWString<int>(particles));
    dos.writeChars("bobView:" + std::string(bobView ? "true" : "false"));
    dos.writeChars("anaglyph3d:" +
                   std::string(anaglyph3d ? "true" : "false"));
    dos.writeChars("advancedOpengl:" +
                   std::string(advancedOpengl ? "true" : "false"));
    dos.writeChars("fpsLimit:" + toWString<int>(framerateLimit));
    dos.writeChars("difficulty:" + toWString<int>(difficulty));
    dos.writeChars("fancyGraphics:" +
                   std::string(fancyGraphics ? "true" : "false"));
    dos.writeChars("ao:" +
                   std::string(ambientOcclusion ? "true" : "false"));
    dos.writeChars("clouds:" + toWString<bool>(renderClouds));
    dos.writeChars("skin:" + skin);
    dos.writeChars("lastServer:" + lastMpIp);

    for (int i = 0; i < keyMappings_length; i++) {
        dos.writeChars("key_" + keyMappings[i]->name + ":" +
                       toWString<int>(keyMappings[i]->key));
    }

    dos.close();
    //    } catch (Exception e) {
    //        System.out.println("Failed to save options");
    //        e.printStackTrace();
    //    }
}

bool Options::isCloudsOn() { return viewDistance < 2 && renderClouds; }
