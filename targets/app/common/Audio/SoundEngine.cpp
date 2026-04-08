#include "SoundEngine.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <vector>

#include "platform/PlatformTypes.h"
#include "platform/PlatformTypes.h"
#include "app/common/Audio/Consoles_SoundEngine.h"
#include "app/linux/Iggy/include/rrCore.h"
#include "app/linux/LinuxGame.h"
#include "platform/C4JThread.h"
#include "platform/fs/fs.h"
#include "java/Random.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/level/storage/LevelData.h"

#if defined(__linux__)
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

// Fixes strcasecmp in miniaudio
// https://stackoverflow.com/questions/31127260/strcasecmp-a-non-standard-function
int strcasecmp(const char* a, const char* b) {
    int ca, cb;
    do {
        ca = *(unsigned char*)a;
        cb = *(unsigned char*)b;
        ca = tolower(toupper(ca));
        cb = tolower(toupper(cb));
        a++;
        b++;
    } while (ca == cb && ca != '\0');
    return ca - cb;
}
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

// stb_vorbis leaks single-letter macros (C, L, R, etc.) that collide with
// identifiers in other translation units during unity builds.
#undef C
#undef L
#undef R
#undef TRUE
#undef FALSE
#endif
#if defined(_WINDOWS64)
#include "app/windows/WindowsGame.h"
#endif

// ASSETS
const char* SoundEngine::m_szStreamFileA[eStream_Max] = {"calm1",
                                                         "calm2",
                                                         "calm3",
                                                         "hal1",
                                                         "hal2",
                                                         "hal3",
                                                         "hal4",
                                                         "nuance1",
                                                         "nuance2",
                                                         "creative1",
                                                         "creative2",
                                                         "creative3",
                                                         "creative4",
                                                         "creative5",
                                                         "creative6",
                                                         "menu1",
                                                         "menu2",
                                                         "menu3",
                                                         "menu4",
                                                         "piano1",
                                                         "piano2",
                                                         "piano3",
                                                         "nether1",
                                                         "nether2",
                                                         "nether3",
                                                         "nether4",
                                                         "the_end_dragon_alive",
                                                         "the_end_end",
                                                         "11",
                                                         "13",
                                                         "blocks",
                                                         "cat",
                                                         "chirp",
                                                         "far",
                                                         "mall",
                                                         "mellohi",
                                                         "stal",
                                                         "strad",
                                                         "ward",
                                                         "where_are_we_now"};
#if defined(__linux__)
char SoundEngine::m_szSoundPath[] = {"app/common/Sound/"};
char SoundEngine::m_szMusicPath[] = {"app/common/"};
char SoundEngine::m_szRedistName[] = {"redist64"};
#endif

#if defined(_WINDOWS64)
char SoundEngine::m_szSoundPath[] = {"Durango\\Sound\\"};
char SoundEngine::m_szMusicPath[] = {"music\\"};
char SoundEngine::m_szRedistName[] = {"redist64"};
#endif
// END ASSETS

// Linux specific functions
#if defined(__linux__)

// PIMPL'd state for the miniaudio backend. Defined here so SoundEngine.h
// stays free of miniaudio.h.
struct SoundEngineMiniAudio {
    ma_engine engine{};
    ma_engine_config engineConfig{};
    ma_sound musicStream{};
};

struct MiniAudioSound {
    ma_sound sound;
    AUDIO_INFO info;
    bool active;
};

SoundEngine::SoundEngine() : m_audio(std::make_unique<SoundEngineMiniAudio>()) {}
SoundEngine::~SoundEngine() = default;
std::vector<MiniAudioSound*> m_activeSounds;
void SoundEngine::init(Options* pOptions) {
    app.DebugPrintf("---SoundEngine::init\n");
    random = new Random();
    *m_audio = SoundEngineMiniAudio{};
    m_musicStreamActive = false;
    m_StreamState = eMusicStreamState_Idle;
    m_iMusicDelay = 0;
    m_validListenerCount = 0;

    m_bHeardTrackA = nullptr;

    // Start the streaming music playing some music from the overworld
    SetStreamingSounds(eStream_Overworld_Calm1, eStream_Overworld_piano3,
                       eStream_Nether1, eStream_Nether4, eStream_end_dragon,
                       eStream_end_end, eStream_CD_1);

    m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);

    m_StreamingAudioInfo.bIs3D = false;
    m_StreamingAudioInfo.x = 0;
    m_StreamingAudioInfo.y = 0;
    m_StreamingAudioInfo.z = 0;
    m_StreamingAudioInfo.volume = 1;
    m_StreamingAudioInfo.pitch = 1;

    memset(CurrentSoundsPlaying, 0,
           sizeof(int) *
               (static_cast<int>(eSoundType_MAX) + static_cast<int>(eSFX_MAX)));
    memset(m_ListenerA, 0, sizeof(AUDIO_LISTENER) * XUSER_MAX_COUNT);
    m_audio->engineConfig = ma_engine_config_init();
    m_audio->engineConfig.listenerCount = MAX_LOCAL_PLAYERS;

    if (ma_engine_init(&m_audio->engineConfig, &m_audio->engine) != MA_SUCCESS) {
        app.DebugPrintf("Failed to initialize miniaudio engine\n");
        return;
    }

    ma_engine_set_volume(&m_audio->engine, 1.0f);

    m_MasterMusicVolume = 1.0f;
    m_MasterEffectsVolume = 1.0f;

    m_validListenerCount = 1;

    m_bSystemMusicPlaying = false;
}
void SoundEngine::destroy() { ma_engine_uninit(&m_audio->engine); }

void SoundEngine::play(int iSound, float x, float y, float z, float volume,
                       float pitch) {
    if (iSound == -1) return;
    char szId[256];
    strncpy(szId, wchSoundNames[iSound], 255);
    for (int i = 0; szId[i]; i++)
        if (szId[i] == '.') szId[i] = '/';

    std::string base = PlatformFilesystem.getBasePath().string() + "/";
    const char* roots[] = {
        "Sound/Minecraft/", "app/common/Sound/Minecraft/",
        "app/common/res/TitleUpdate/res/Sound/Minecraft/"};
    char finalPath[512] = {0};
    bool found = false;

    for (const char* root : roots) {
        std::string fullRoot = base + root;
        for (const char* ext : {".ogg", ".wav"}) {
            int count = 0;
            for (int i = 1; i <= 16; i++) {
                char tryP[512];
                snprintf(tryP, 512, "%s%s%d%s", fullRoot.c_str(), szId, i, ext);
                if (PlatformFilesystem.exists(tryP))
                    count = i;
                else
                    break;
            }
            if (count > 0) {
                snprintf(finalPath, 512, "%s%s%d%s", fullRoot.c_str(), szId,
                         (rand() % count) + 1, ext);
                found = true;
                break;
            }
            char tryP[512];
            snprintf(tryP, 512, "%s%s%s", fullRoot.c_str(), szId, ext);
            if (PlatformFilesystem.exists(tryP)) {
                strncpy(finalPath, tryP, 511);
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) return;
    MiniAudioSound* s = new MiniAudioSound();
    memset(&s->info, 0, sizeof(AUDIO_INFO));
    s->info.x = x;
    s->info.y = y;
    s->info.z = z;
    s->info.volume = volume;
    s->info.pitch = pitch;
    s->info.bIs3D = true;

    if (ma_sound_init_from_file(&m_audio->engine, finalPath, MA_SOUND_FLAG_ASYNC,
                                nullptr, nullptr, &s->sound) == MA_SUCCESS) {
        ma_sound_set_spatialization_enabled(&s->sound, MA_TRUE);
        ma_sound_set_min_distance(&s->sound, 2.0f);
        ma_sound_set_max_distance(&s->sound, 48.0f);
        ma_sound_set_volume(&s->sound, volume * m_MasterEffectsVolume);
        ma_sound_set_position(&s->sound, x, y, z);
        ma_sound_start(&s->sound);
        m_activeSounds.push_back(s);
    } else
        delete s;
}

void SoundEngine::playUI(int iSound, float volume, float pitch) {
    char szIdentifier[256];
    if (iSound >= eSFX_MAX)
        strncpy(szIdentifier, wchSoundNames[iSound], 255);
    else
        strncpy(szIdentifier, wchUISoundNames[iSound], 255);
    for (int i = 0; szIdentifier[i]; i++)
        if (szIdentifier[i] == '.') szIdentifier[i] = '/';
    std::string base = PlatformFilesystem.getBasePath().string() + "/";
    const char* roots[] = {
        "Sound/Minecraft/UI/",
        "Sound/Minecraft/",
        "app/common/Sound/Minecraft/UI/",
        "app/common/Sound/Minecraft/",
    };
    char finalPath[512] = {0};
    bool found = false;

    for (const char* root : roots) {
        for (const char* ext : {".ogg", ".wav", ".mp3"}) {
            char tryP[512];
            snprintf(tryP, 512, "%s%s%s%s", base.c_str(), root, szIdentifier,
                     ext);
            if (PlatformFilesystem.exists(tryP)) {
                strncpy(finalPath, tryP, 511);
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) return;
    MiniAudioSound* s = new MiniAudioSound();
    memset(&s->info, 0, sizeof(AUDIO_INFO));
    s->info.volume = volume;
    s->info.pitch = pitch;
    s->info.bIs3D = false;

    if (ma_sound_init_from_file(&m_audio->engine, finalPath, MA_SOUND_FLAG_ASYNC,
                                nullptr, nullptr, &s->sound) == MA_SUCCESS) {
        ma_sound_set_spatialization_enabled(&s->sound, MA_FALSE);
        ma_sound_set_volume(&s->sound, volume * m_MasterEffectsVolume);
        ma_sound_set_pitch(&s->sound, pitch);
        ma_sound_start(&s->sound);
        m_activeSounds.push_back(s);
    } else
        delete s;
}

int SoundEngine::getMusicID(int iDomain) {
    int iRandomVal = 0;
    Minecraft* pMinecraft = Minecraft::GetInstance();

    // Protection from errors
    if (pMinecraft == nullptr || pMinecraft->skins == nullptr) {
        // any track from the overworld
        return GetRandomishTrack(m_iStream_Overworld_Min,
                                 m_iStream_Overworld_Max);
    }

    if (pMinecraft->skins->isUsingDefaultSkin()) {
        switch (iDomain) {
            case LevelData::DIMENSION_END:
                // the end isn't random - it has different music depending
                // whether the dragon is alive or not, but we've not
                // added the dead dragon music yet
                // haha they said wheter
                return m_iStream_End_Min;
            case LevelData::DIMENSION_NETHER:
                return GetRandomishTrack(m_iStream_Nether_Min,
                                         m_iStream_Nether_Max);
                // return m_iStream_Nether_Min +
                // random->nextInt(m_iStream_Nether_Max-m_iStream_Nether_Min);
            default:  // overworld
                // return m_iStream_Overworld_Min +
                // random->nextInt(m_iStream_Overworld_Max-m_iStream_Overworld_Min);
                return GetRandomishTrack(m_iStream_Overworld_Min,
                                         m_iStream_Overworld_Max);
        }
    } else {
        // using a texture pack - may have multiple End music tracks
        switch (iDomain) {
            case LevelData::DIMENSION_END:
                return GetRandomishTrack(m_iStream_End_Min, m_iStream_End_Max);
            case LevelData::DIMENSION_NETHER:
                // return m_iStream_Nether_Min +
                // random->nextInt(m_iStream_Nether_Max-m_iStream_Nether_Min);
                return GetRandomishTrack(m_iStream_Nether_Min,
                                         m_iStream_Nether_Max);
            default:  // overworld
                // return m_iStream_Overworld_Min +
                // random->nextInt(m_iStream_Overworld_Max-m_iStream_Overworld_Min);
                return GetRandomishTrack(m_iStream_Overworld_Min,
                                         m_iStream_Overworld_Max);
        }
    }
}

int SoundEngine::getMusicID(const std::string& name) {
    int iCD = 0;
    for (size_t i = 0; i < 12; i++) {
        std::string fileName = m_szStreamFileA[i + eStream_CD_1];

        if (name == fileName) {
            iCD = static_cast<int>(i);
            break;
        }
    }
    return iCD + m_iStream_CD_1;
}

void SoundEngine::playStreaming(const std::string& name, float x, float y,
                                float z, float volume, float pitch,
                                bool bMusicDelay) {
    m_StreamingAudioInfo.x = x;
    m_StreamingAudioInfo.y = y;
    m_StreamingAudioInfo.z = z;
    m_StreamingAudioInfo.volume = volume;
    m_StreamingAudioInfo.pitch = pitch;

    if (m_StreamState == eMusicStreamState_Playing) {
        m_StreamState = eMusicStreamState_Stop;
    } else if (m_StreamState == eMusicStreamState_Opening) {
        m_StreamState = eMusicStreamState_OpeningCancel;
    }
    app.DebugPrintf("playStreaming %S", name.c_str());
    if (name.empty()) {
        // music, or stop CD
        m_StreamingAudioInfo.bIs3D = false;

        // we need a music id
        // random delay of up to 3 minutes for music
        m_iMusicDelay = random->nextInt(
            20 * 60 * 3);  // random->nextInt(20 * 60 * 10) + 20 * 60 * 10;

#if defined(_DEBUG)
        m_iMusicDelay = 0;
#endif
        Minecraft* pMinecraft = Minecraft::GetInstance();

        bool playerInEnd = false;
        bool playerInNether = false;

        for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            if (pMinecraft->localplayers[i] != nullptr) {
                if (pMinecraft->localplayers[i]->dimension ==
                    LevelData::DIMENSION_END) {
                    playerInEnd = true;
                } else if (pMinecraft->localplayers[i]->dimension ==
                           LevelData::DIMENSION_NETHER) {
                    playerInNether = true;
                }
            }
        }
        if (playerInEnd) {
            m_musicID = getMusicID(LevelData::DIMENSION_END);
        } else if (playerInNether) {
            m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
        } else {
            m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
        }
    } else {
        // jukebox
        m_StreamingAudioInfo.bIs3D = true;
        m_musicID = getMusicID(name);
        m_iMusicDelay = 0;
    }
}
int SoundEngine::OpenStreamThreadProc(void* lpParameter) {
    SoundEngine* soundEngine = (SoundEngine*)lpParameter;

    const char* ext = strrchr(soundEngine->m_szStreamName, '.');

    if (soundEngine->m_musicStreamActive) {
        ma_sound_stop(&soundEngine->m_audio->musicStream);
        ma_sound_uninit(&soundEngine->m_audio->musicStream);
        soundEngine->m_musicStreamActive = false;
    }

    ma_result result = ma_sound_init_from_file(
        &soundEngine->m_audio->engine, soundEngine->m_szStreamName,
        MA_SOUND_FLAG_STREAM, nullptr, nullptr, &soundEngine->m_audio->musicStream);

    if (result != MA_SUCCESS) {
        app.DebugPrintf(
            "SoundEngine::OpenStreamThreadProc - Failed to open stream: "
            "%s\n",
            soundEngine->m_szStreamName);
        return 0;
    }

    ma_sound_set_spatialization_enabled(&soundEngine->m_audio->musicStream, MA_FALSE);
    ma_sound_set_looping(&soundEngine->m_audio->musicStream, MA_FALSE);

    soundEngine->m_musicStreamActive = true;

    return 0;
}
void SoundEngine::playMusicTick() {
    static float fMusicVol = 0.0f;
    fMusicVol = getMasterMusicVolume();

    switch (m_StreamState) {
        case eMusicStreamState_Idle:
            if (m_iMusicDelay > 0) {
                m_iMusicDelay--;
                return;
            }
            if (m_musicID != -1) {
                std::string base = PlatformFilesystem.getBasePath().string() + "/";
                bool isCD = (m_musicID >= m_iStream_CD_1);
                const char* folder = isCD ? "cds/" : "music/";
                const char* track = m_szStreamFileA[m_musicID];
                bool found = false;
                m_szStreamName[0] = '\0';

                const char* roots[] = {"app/common/music/",
                                       "music/", "./"};

                for (const char* r : roots) {
                    for (const char* e : {".ogg", ".mp3", ".wav"}) {
                        // try with folder prefix (music/ or cds/)
                        snprintf(m_szStreamName, sizeof(m_szStreamName), "%s%s%s%s%s", base.c_str(), r, folder,
                                 track, e);
                        if (PlatformFilesystem.exists(m_szStreamName)) {
                            found = true;
                            break;
                        }
                        // try without folder prefix
                        snprintf(m_szStreamName, sizeof(m_szStreamName), "%s%s%s%s", base.c_str(), r, track, e);
                        if (PlatformFilesystem.exists(m_szStreamName)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }

                if (found) {
                    SetIsPlayingStreamingGameMusic(!isCD);
                    SetIsPlayingStreamingCDMusic(isCD);
                    m_openStreamThread = new C4JThread(
                        OpenStreamThreadProc, this, "OpenStreamThreadProc");
                    m_openStreamThread->run();
                    m_StreamState = eMusicStreamState_Opening;
                } else {
                    app.DebugPrintf(
                        "[SoundEngine] oh noes couldn't find music track '%s', "
                        "retrying "
                        "in 1min\n",
                        track);
                    m_iMusicDelay = 20 * 60;
                }
            }
            break;

        case eMusicStreamState_Opening:
            if (!m_openStreamThread->isRunning()) {
                delete m_openStreamThread;
                m_openStreamThread = nullptr;

                if (!m_musicStreamActive) {
                    m_StreamState = eMusicStreamState_Idle;
                    break;
                }

                ma_sound_set_spatialization_enabled(
                    &m_audio->musicStream,
                    m_StreamingAudioInfo.bIs3D ? MA_TRUE : MA_FALSE);
                if (m_StreamingAudioInfo.bIs3D) {
                    ma_sound_set_position(
                        &m_audio->musicStream, m_StreamingAudioInfo.x,
                        m_StreamingAudioInfo.y, m_StreamingAudioInfo.z);
                }

                ma_sound_set_pitch(&m_audio->musicStream, m_StreamingAudioInfo.pitch);
                ma_sound_set_volume(
                    &m_audio->musicStream,
                    m_StreamingAudioInfo.volume * getMasterMusicVolume());
                ma_sound_start(&m_audio->musicStream);

                m_StreamState = eMusicStreamState_Playing;
            }
            break;

        case eMusicStreamState_OpeningCancel:
            if (!m_openStreamThread->isRunning()) {
                delete m_openStreamThread;
                m_openStreamThread = nullptr;
                m_StreamState = eMusicStreamState_Stop;
            }
            break;

        case eMusicStreamState_Stop:
            if (m_musicStreamActive) {
                ma_sound_stop(&m_audio->musicStream);
                ma_sound_uninit(&m_audio->musicStream);
                m_musicStreamActive = false;
            }
            SetIsPlayingStreamingCDMusic(false);
            SetIsPlayingStreamingGameMusic(false);
            m_StreamState = eMusicStreamState_Idle;
            break;

        case eMusicStreamState_Playing:
            if (GetIsPlayingStreamingGameMusic()) {
                bool playerInEnd = false, playerInNether = false;
                Minecraft* pMinecraft = Minecraft::GetInstance();

                for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; ++i) {
                    if (pMinecraft->localplayers[i]) {
                        if (pMinecraft->localplayers[i]->dimension ==
                            LevelData::DIMENSION_END)
                            playerInEnd = true;
                        else if (pMinecraft->localplayers[i]->dimension ==
                                 LevelData::DIMENSION_NETHER)
                            playerInNether = true;
                    }
                }

                // Handle Dimension Switching
                bool needsStop = false;
                if (playerInEnd && !GetIsPlayingEndMusic()) {
                    m_musicID = getMusicID(LevelData::DIMENSION_END);
                    SetIsPlayingEndMusic(true);
                    SetIsPlayingNetherMusic(false);
                    needsStop = true;
                } else if (!playerInEnd && GetIsPlayingEndMusic()) {
                    m_musicID =
                        playerInNether
                            ? getMusicID(LevelData::DIMENSION_NETHER)
                            : getMusicID(LevelData::DIMENSION_OVERWORLD);
                    SetIsPlayingEndMusic(false);
                    SetIsPlayingNetherMusic(playerInNether);
                    needsStop = true;
                } else if (playerInNether && !GetIsPlayingNetherMusic()) {
                    m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
                    SetIsPlayingNetherMusic(true);
                    SetIsPlayingEndMusic(false);
                    needsStop = true;
                } else if (!playerInNether && GetIsPlayingNetherMusic()) {
                    m_musicID =
                        playerInEnd
                            ? getMusicID(LevelData::DIMENSION_END)
                            : getMusicID(LevelData::DIMENSION_OVERWORLD);
                    SetIsPlayingNetherMusic(false);
                    SetIsPlayingEndMusic(playerInEnd);
                    needsStop = true;
                }

                if (needsStop) m_StreamState = eMusicStreamState_Stop;

                // volume change required?
                if (m_musicStreamActive)
                    ma_sound_set_volume(
                        &m_audio->musicStream,
                        m_StreamingAudioInfo.volume * fMusicVol);

            } else if (m_StreamingAudioInfo.bIs3D && m_validListenerCount > 1 &&
                       m_musicStreamActive) {
                float fClosestDist = 1e6f;
                int iClosest = 0;
                for (size_t i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                    if (m_ListenerA[i].bValid) {
                        float dist = sqrtf(powf(m_StreamingAudioInfo.x -
                                                    m_ListenerA[i].vPosition.x,
                                                2) +
                                           powf(m_StreamingAudioInfo.y -
                                                    m_ListenerA[i].vPosition.y,
                                                2) +
                                           powf(m_StreamingAudioInfo.z -
                                                    m_ListenerA[i].vPosition.z,
                                                2));
                        if (dist < fClosestDist) {
                            fClosestDist = dist;
                            iClosest = i;
                        }
                    }
                }
                ma_sound_set_position(
                    &m_audio->musicStream,
                    m_StreamingAudioInfo.x - m_ListenerA[iClosest].vPosition.x,
                    m_StreamingAudioInfo.y - m_ListenerA[iClosest].vPosition.y,
                    m_StreamingAudioInfo.z - m_ListenerA[iClosest].vPosition.z);
            }
            break;

        case eMusicStreamState_Completed:
            m_iMusicDelay = random->nextInt(20 * 60 * 3);
            {
                int dim = LevelData::DIMENSION_OVERWORLD;
                Minecraft* pMc = Minecraft::GetInstance();
                for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                    if (pMc->localplayers[i]) {
                        dim = pMc->localplayers[i]->dimension;
                        break;
                    }
                }
                m_musicID = getMusicID(dim);
                SetIsPlayingEndMusic(dim == LevelData::DIMENSION_END);
                SetIsPlayingNetherMusic(dim == LevelData::DIMENSION_NETHER);
            }
            m_StreamState = eMusicStreamState_Idle;
            break;
    }

    // check the status of the stream - this is for when a track completes
    // rather than is stopped by the user action

    if (m_musicStreamActive && !ma_sound_is_playing(&m_audio->musicStream) &&
        ma_sound_at_end(&m_audio->musicStream)) {
        ma_sound_uninit(&m_audio->musicStream);
        m_musicStreamActive = false;
        SetIsPlayingStreamingCDMusic(false);
        SetIsPlayingStreamingGameMusic(false);
        m_StreamState = eMusicStreamState_Completed;
    }
}

void SoundEngine::updateMiniAudio() {
    if (m_validListenerCount == 1) {
        for (size_t i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            if (m_ListenerA[i].bValid) {
                ma_engine_listener_set_position(
                    &m_audio->engine, 0, m_ListenerA[i].vPosition.x,
                    m_ListenerA[i].vPosition.y, m_ListenerA[i].vPosition.z);

                ma_engine_listener_set_direction(&m_audio->engine, 0,
                                                 m_ListenerA[i].vOrientFront.x,
                                                 m_ListenerA[i].vOrientFront.y,
                                                 m_ListenerA[i].vOrientFront.z);

                ma_engine_listener_set_world_up(&m_audio->engine, 0, 0.0f, 1.0f, 0.0f);

                break;
            }
        }
    } else {
        ma_engine_listener_set_position(&m_audio->engine, 0, 0.0f, 0.0f, 0.0f);
        ma_engine_listener_set_direction(&m_audio->engine, 0, 0.0f, 0.0f, 1.0f);
        ma_engine_listener_set_world_up(&m_audio->engine, 0, 0.0f, 1.0f, 0.0f);
    }

    for (auto it = m_activeSounds.begin(); it != m_activeSounds.end();) {
        MiniAudioSound* s = *it;

        if (!ma_sound_is_playing(&s->sound)) {
            ma_sound_uninit(&s->sound);
            delete s;
            it = m_activeSounds.erase(it);
            continue;
        }

        float finalVolume =
            s->info.volume * m_MasterEffectsVolume * SFX_VOLUME_MULTIPLIER;
        if (finalVolume > SFX_MAX_GAIN) finalVolume = SFX_MAX_GAIN;

        ma_sound_set_volume(&s->sound, finalVolume);
        ma_sound_set_pitch(&s->sound, s->info.pitch);

        if (s->info.bIs3D) {
            if (m_validListenerCount > 1) {
                float fClosest = 10000.0f;
                int iClosestListener = 0;
                float fClosestX = 0.0f, fClosestY = 0.0f, fClosestZ = 0.0f,
                      fDist;
                for (size_t i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                    if (m_ListenerA[i].bValid) {
                        float x, y, z;

                        x = fabs(m_ListenerA[i].vPosition.x - s->info.x);
                        y = fabs(m_ListenerA[i].vPosition.y - s->info.y);
                        z = fabs(m_ListenerA[i].vPosition.z - s->info.z);
                        fDist = x + y + z;

                        if (fDist < fClosest) {
                            fClosest = fDist;
                            fClosestX = x;
                            fClosestY = y;
                            fClosestZ = z;
                            iClosestListener = i;
                        }
                    }
                }

                float realDist =
                    sqrtf((fClosestX * fClosestX) + (fClosestY * fClosestY) +
                          (fClosestZ * fClosestZ));
                ma_sound_set_position(&s->sound, 0, 0, realDist);
            } else {
                ma_sound_set_position(&s->sound, s->info.x, s->info.y,
                                      s->info.z);
            }
        }

        ++it;
    }
}

void SoundEngine::tick(std::shared_ptr<Mob>* players, float a) {
    // update the listener positions
    int listenerCount = 0;
    if (players) {
        bool bListenerPostionSet = false;
        for (size_t i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            if (players[i] != nullptr) {
                m_ListenerA[i].bValid = true;
                F32 x, y, z;
                x = players[i]->xo + (players[i]->x - players[i]->xo) * a;
                y = players[i]->yo + (players[i]->y - players[i]->yo) * a;
                z = players[i]->zo + (players[i]->z - players[i]->zo) * a;

                float yRot = players[i]->yRotO +
                             (players[i]->yRot - players[i]->yRotO) * a;
                float yCos = (float)cos(yRot * Mth::DEG_TO_RAD);
                float ySin = (float)sin(yRot * Mth::DEG_TO_RAD);

                // store the listener positions for splitscreen
                m_ListenerA[i].vPosition.x = x;
                m_ListenerA[i].vPosition.y = y;
                m_ListenerA[i].vPosition.z = z;

                m_ListenerA[i].vOrientFront.x = -ySin;
                m_ListenerA[i].vOrientFront.y = 0;
                m_ListenerA[i].vOrientFront.z = yCos;

                listenerCount++;
            } else {
                m_ListenerA[i].bValid = false;
            }
        }
    }

    // If there were no valid players set, make up a default listener
    if (listenerCount == 0) {
        m_ListenerA[0].vPosition.x = 0;
        m_ListenerA[0].vPosition.y = 0;
        m_ListenerA[0].vPosition.z = 0;
        m_ListenerA[0].vOrientFront.x = 0;
        m_ListenerA[0].vOrientFront.y = 0;
        m_ListenerA[0].vOrientFront.z = 1.0f;
        listenerCount++;
    }
    m_validListenerCount = listenerCount;
    updateMiniAudio();
}
// Classic sound module
#else
void SoundEngine::init(Options* pOptions) {
    app.DebugPrintf("---SoundEngine::init\n");
#if defined(__DISABLE_MILES__)
    return;
#endif

    char* redistpath;

#if defined(_WINDOWS64)
    redistpath = AIL_set_redist_directory(m_szRedistName);
#endif

    app.DebugPrintf("---SoundEngine::init - AIL_startup\n");
    S32 ret = AIL_startup();

    int iNumberOfChannels = initAudioHardware(8);

    // Create a driver to render our audio - 44khz, 16 bit,
    m_hDriver = AIL_open_digital_driver(44100, 16, MSS_MC_USE_SYSTEM_CONFIG, 0);
    if (m_hDriver == 0) {
        app.DebugPrintf("Couldn't open digital sound driver. (%s)\n",
                        AIL_last_error());
        AIL_shutdown();
        return;
    }
    app.DebugPrintf("---SoundEngine::init - driver opened\n");

    AIL_set_event_error_callback(ErrorCallback);

    AIL_set_3D_rolloff_factor(m_hDriver, 1.0);

    // Create an event system tied to that driver - let Miles choose memory
    // defaults.
    // if (AIL_startup_event_system(m_hDriver, 0, 0, 0) == 0)
    // 4J-PB - Durango complains that the default memory (64k)isn't enough
    // Error: MilesEvent: Out of event system memory (pool passed to event
    // system startup exhausted). AP - increased command buffer from the default
    // 5K to 20K for Vita

    if (AIL_startup_event_system(m_hDriver, 1024 * 20, 0, 1024 * 128) == 0) {
        app.DebugPrintf("Couldn't init event system (%s).\n", AIL_last_error());
        AIL_close_digital_driver(m_hDriver);
        AIL_shutdown();
        app.DebugPrintf(
            "---SoundEngine::init - AIL_startup_event_system failed\n");
        return;
    }
    char szBankName[255];
    strcpy((char*)szBankName, m_szSoundPath);

    strcat((char*)szBankName, "Minecraft.msscmp");

    m_hBank = AIL_add_soundbank(szBankName, 0);

    if (m_hBank == nullptr) {
        char* Error = AIL_last_error();
        app.DebugPrintf("Couldn't open soundbank: %s (%s)\n", szBankName,
                        Error);
        AIL_close_digital_driver(m_hDriver);
        AIL_shutdown();
        return;
    }

    // #ifdef _DEBUG
    HMSSENUM token = MSS_FIRST;
    char const* Events[1] = {0};
    S32 EventCount = 0;
    while (AIL_enumerate_events(m_hBank, &token, 0, &Events[0])) {
        app.DebugPrintf(4, "%d - %s\n", EventCount, Events[0]);

        EventCount++;
    }
    // #endif

    U64 u64Result;
    u64Result = AIL_enqueue_event_by_name("Minecraft/CacheSounds");

    m_MasterMusicVolume = 1.0f;
    m_MasterEffectsVolume = 1.0f;

    // AIL_set_variable_float(0,"UserEffectVol",1);

    m_bSystemMusicPlaying = false;

    m_openStreamThread = nullptr;
}

// AP - moved to a separate function so it can be called from the mixer callback
// on Vita
void SoundEngine::updateMiles() {
    if (m_validListenerCount == 1) {
        for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            // set the listener as the first player we find
            if (m_ListenerA[i].bValid) {
                AIL_set_listener_3D_position(
                    m_hDriver, m_ListenerA[i].vPosition.x,
                    m_ListenerA[i].vPosition.y,
                    -m_ListenerA[i]
                         .vPosition.z);  // Flipped sign of z as Miles is
                                         // expecting left handed coord system
                AIL_set_listener_3D_orientation(
                    m_hDriver, -m_ListenerA[i].vOrientFront.x,
                    m_ListenerA[i].vOrientFront.y,
                    m_ListenerA[i].vOrientFront.z, 0, 1,
                    0);  // Flipped sign of z as Miles is expecting left handed
                         // coord system
                break;
            }
        }
    } else {
        // 4J-PB - special case for splitscreen
        // the shortest distance between any listener and a sound will be used
        // to play a sound a set distance away down the z axis. The listener
        // position will be set to 0,0,0, and the orientation will be facing
        // down the z axis

        AIL_set_listener_3D_position(m_hDriver, 0, 0, 0);
        AIL_set_listener_3D_orientation(m_hDriver, 0, 0, 1, 0, 1, 0);
    }

    AIL_begin_event_queue_processing();

    // Iterate over the sounds
    S32 StartedCount = 0, CompletedCount = 0, TotalCount = 0;
    HMSSENUM token = MSS_FIRST;
    MILESEVENTSOUNDINFO SoundInfo;
    int Playing = 0;
    while (AIL_enumerate_sound_instances(0, &token, 0, 0, 0, &SoundInfo)) {
        AUDIO_INFO* game_data = (AUDIO_INFO*)(SoundInfo.UserBuffer);

        if (SoundInfo.Status == MILESEVENT_SOUND_STATUS_PLAYING) {
            Playing += 1;
        }

        if (SoundInfo.Status != MILESEVENT_SOUND_STATUS_COMPLETE) {
            // apply the master volume
            // watch for the 'special' volume levels
            bool isThunder = false;
            if (game_data->volume == 10000.0f) {
                isThunder = true;
            }
            if (game_data->volume > 1) {
                game_data->volume = 1;
            }
            AIL_set_sample_volume_levels(
                SoundInfo.Sample, game_data->volume * m_MasterEffectsVolume,
                game_data->volume * m_MasterEffectsVolume);

            float distanceScaler = 16.0f;
            switch (SoundInfo.Status) {
                case MILESEVENT_SOUND_STATUS_PENDING:
                    // 4J-PB - causes the falloff to be calculated on the PPU
                    // instead of the SPU, and seems to resolve our distorted
                    // sound issue
                    AIL_register_falloff_function_callback(
                        SoundInfo.Sample, &custom_falloff_function);

                    if (game_data->bIs3D) {
                        AIL_set_sample_is_3D(SoundInfo.Sample, 1);

                        int iSound = game_data->iSound - eSFX_MAX;
                        switch (iSound) {
                            // Is this the Dragon?
                            case eSoundType_MOB_ENDERDRAGON_GROWL:
                            case eSoundType_MOB_ENDERDRAGON_MOVE:
                            case eSoundType_MOB_ENDERDRAGON_END:
                            case eSoundType_MOB_ENDERDRAGON_HIT:
                                distanceScaler = 100.0f;
                                break;
                            case eSoundType_FIREWORKS_BLAST:
                            case eSoundType_FIREWORKS_BLAST_FAR:
                            case eSoundType_FIREWORKS_LARGE_BLAST:
                            case eSoundType_FIREWORKS_LARGE_BLAST_FAR:
                                distanceScaler = 100.0f;
                                break;
                            case eSoundType_MOB_GHAST_MOAN:
                            case eSoundType_MOB_GHAST_SCREAM:
                            case eSoundType_MOB_GHAST_DEATH:
                            case eSoundType_MOB_GHAST_CHARGE:
                            case eSoundType_MOB_GHAST_FIREBALL:
                                distanceScaler = 30.0f;
                                break;
                        }

                        // Set a special distance scaler for thunder, which we
                        // respond to by having no attenutation
                        if (isThunder) {
                            distanceScaler = 10000.0f;
                        }
                    } else {
                        AIL_set_sample_is_3D(SoundInfo.Sample, 0);
                    }

                    AIL_set_sample_3D_distances(SoundInfo.Sample,
                                                distanceScaler, 1, 0);
                    // set the pitch
                    if (!game_data->bUseSoundsPitchVal) {
                        AIL_set_sample_playback_rate_factor(SoundInfo.Sample,
                                                            game_data->pitch);
                    }

                    if (game_data->bIs3D) {
                        if (m_validListenerCount > 1) {
                            float fClosest = 10000.0f;
                            int iClosestListener = 0;
                            float fClosestX = 0.0f, fClosestY = 0.0f,
                                  fClosestZ = 0.0f, fDist;
                            // need to calculate the distance from the sound to
                            // the nearest listener - use Manhattan Distance as
                            // the decision
                            for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                                if (m_ListenerA[i].bValid) {
                                    float x, y, z;

                                    x = fabs(m_ListenerA[i].vPosition.x -
                                             game_data->x);
                                    y = fabs(m_ListenerA[i].vPosition.y -
                                             game_data->y);
                                    z = fabs(m_ListenerA[i].vPosition.z -
                                             game_data->z);
                                    fDist = x + y + z;

                                    if (fDist < fClosest) {
                                        fClosest = fDist;
                                        fClosestX = x;
                                        fClosestY = y;
                                        fClosestZ = z;
                                        iClosestListener = i;
                                    }
                                }
                            }

                            // our distances in the world aren't very big, so
                            // floats rather than casts to doubles should be
                            // fine
                            fDist = sqrtf((fClosestX * fClosestX) +
                                          (fClosestY * fClosestY) +
                                          (fClosestZ * fClosestZ));
                            AIL_set_sample_3D_position(SoundInfo.Sample, 0, 0,
                                                       fDist);

                            // app.DebugPrintf("Playing sound %d %f from nearest
                            // listener
                            // [%d]\n",SoundInfo.EventID,fDist,iClosestListener);
                        } else {
                            AIL_set_sample_3D_position(
                                SoundInfo.Sample, game_data->x, game_data->y,
                                -game_data->z);  // Flipped sign of z as Miles
                                                 // is expecting left handed
                                                 // coord system
                        }
                    }
                    break;

                default:
                    if (game_data->bIs3D) {
                        if (m_validListenerCount > 1) {
                            float fClosest = 10000.0f;
                            int iClosestListener = 0;
                            float fClosestX = 0.0f, fClosestY = 0.0f,
                                  fClosestZ = 0.0f, fDist;
                            // need to calculate the distance from the sound to
                            // the nearest listener - use Manhattan Distance as
                            // the decision
                            for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                                if (m_ListenerA[i].bValid) {
                                    float x, y, z;

                                    x = fabs(m_ListenerA[i].vPosition.x -
                                             game_data->x);
                                    y = fabs(m_ListenerA[i].vPosition.y -
                                             game_data->y);
                                    z = fabs(m_ListenerA[i].vPosition.z -
                                             game_data->z);
                                    fDist = x + y + z;

                                    if (fDist < fClosest) {
                                        fClosest = fDist;
                                        fClosestX = x;
                                        fClosestY = y;
                                        fClosestZ = z;
                                        iClosestListener = i;
                                    }
                                }
                            }
                            // our distances in the world aren't very big, so
                            // floats rather than casts to doubles should be
                            // fine
                            fDist = sqrtf((fClosestX * fClosestX) +
                                          (fClosestY * fClosestY) +
                                          (fClosestZ * fClosestZ));
                            AIL_set_sample_3D_position(SoundInfo.Sample, 0, 0,
                                                       fDist);

                            // app.DebugPrintf("Playing sound %d %f from nearest
                            // listener
                            // [%d]\n",SoundInfo.EventID,fDist,iClosestListener);
                        } else {
                            AIL_set_sample_3D_position(
                                SoundInfo.Sample, game_data->x, game_data->y,
                                -game_data->z);  // Flipped sign of z as Miles
                                                 // is expecting left handed
                                                 // coord system
                        }
                    }
                    break;
            }
        }
    }
    AIL_complete_event_queue_processing();
}

// #define DISTORTION_TEST
#if defined(DISTORTION_TEST)
static float fVal = 0.0f;
#endif
/////////////////////////////////////////////
//
//	tick
//
/////////////////////////////////////////////

void SoundEngine::tick(std::shared_ptr<Mob>* players, float a) {
#if defined(__DISABLE_MILES__)
    return;
#endif

    // update the listener positions
    int listenerCount = 0;
#if defined(DISTORTION_TEST)
    float fX, fY, fZ;
#endif
    if (players) {
        bool bListenerPostionSet = false;
        for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            if (players[i] != nullptr) {
                m_ListenerA[i].bValid = true;
                F32 x, y, z;
                x = players[i]->xo + (players[i]->x - players[i]->xo) * a;
                y = players[i]->yo + (players[i]->y - players[i]->yo) * a;
                z = players[i]->zo + (players[i]->z - players[i]->zo) * a;

                float yRot = players[i]->yRotO +
                             (players[i]->yRot - players[i]->yRotO) * a;
                float yCos =
                    (float)cos(-yRot * Mth::DEG_TO_RAD - std::numbers::pi);
                float ySin =
                    (float)sin(-yRot * Mth::DEG_TO_RAD - std::numbers::pi);

                // store the listener positions for splitscreen
                m_ListenerA[i].vPosition.x = x;
                m_ListenerA[i].vPosition.y = y;
                m_ListenerA[i].vPosition.z = z;

                m_ListenerA[i].vOrientFront.x = ySin;
                m_ListenerA[i].vOrientFront.y = 0;
                m_ListenerA[i].vOrientFront.z = yCos;

                listenerCount++;
            } else {
                m_ListenerA[i].bValid = false;
            }
        }
    }

    // If there were no valid players set, make up a default listener
    if (listenerCount == 0) {
        m_ListenerA[0].vPosition.x = 0;
        m_ListenerA[0].vPosition.y = 0;
        m_ListenerA[0].vPosition.z = 0;
        m_ListenerA[0].vOrientFront.x = 0;
        m_ListenerA[0].vOrientFront.y = 0;
        m_ListenerA[0].vOrientFront.z = 1.0f;
        listenerCount++;
    }
    m_validListenerCount = listenerCount;

    updateMiles();
}
SoundEngine::SoundEngine() {
    random = new Random();
    m_hStream = 0;
    m_StreamState = eMusicStreamState_Idle;
    m_iMusicDelay = 0;
    m_validListenerCount = 0;

    m_bHeardTrackA = nullptr;

    // Start the streaming music playing some music from the overworld
    SetStreamingSounds(eStream_Overworld_Calm1, eStream_Overworld_piano3,
                       eStream_Nether1, eStream_Nether4, eStream_end_dragon,
                       eStream_end_end, eStream_CD_1);

    m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);

    m_StreamingAudioInfo.bIs3D = false;
    m_StreamingAudioInfo.x = 0;
    m_StreamingAudioInfo.y = 0;
    m_StreamingAudioInfo.z = 0;
    m_StreamingAudioInfo.volume = 1;
    m_StreamingAudioInfo.pitch = 1;

    memset(CurrentSoundsPlaying, 0, sizeof(int) * (eSoundType_MAX + eSFX_MAX));
    memset(m_ListenerA, 0, sizeof(AUDIO_LISTENER) * XUSER_MAX_COUNT);
}

void SoundEngine::destroy() {}
#if defined(_DEBUG)
void SoundEngine::GetSoundName(char* szSoundName, int iSound) {
    strcpy((char*)szSoundName, "Minecraft/");
    std::string name = wchSoundNames[iSound];
    char* SoundName = (char*)ConvertSoundPathToName(name);
    strcat((char*)szSoundName, SoundName);
}
#endif
/////////////////////////////////////////////
//
//	play
//
/////////////////////////////////////////////
void SoundEngine::play(int iSound, float x, float y, float z, float volume,
                       float pitch) {
    U8 szSoundName[256];

    if (iSound == -1) {
        app.DebugPrintf(6, "PlaySound with sound of -1 !!!!!!!!!!!!!!!\n");
        return;
    }

    // AP removed old counting system. Now relying on Miles' Play Count Limit
    /*	// if we are already playing loads of this sounds ignore this one
    if(CurrentSoundsPlaying[iSound+eSFX_MAX]>MAX_SAME_SOUNDS_PLAYING)
    {
    // 		std::string name = wchSoundNames[iSound];
    // 		char *SoundName = (char *)ConvertSoundPathToName(name);
    // 		app.DebugPrintf("Too many %s sounds playing!\n",SoundName);
    return;
    }*/

    // if (iSound != eSoundType_MOB_IRONGOLEM_WALK) return;

    // build the name
    strcpy((char*)szSoundName, "Minecraft/");

#if defined(DISTORTION_TEST)
    std::string name = wchSoundNames[eSoundType_MOB_ENDERDRAGON_GROWL];
#else
    std::string name = wchSoundNames[iSound];
#endif

    char* SoundName = (char*)ConvertSoundPathToName(name);
    strcat((char*)szSoundName, SoundName);

    //	app.DebugPrintf(6,"PlaySound - %d - %s - %s (%f %f %f, vol %f, pitch
    //%f)\n",iSound, SoundName, szSoundName,x,y,z,volume,pitch);

    AUDIO_INFO AudioInfo;
    AudioInfo.x = x;
    AudioInfo.y = y;
    AudioInfo.z = z;
    AudioInfo.volume = volume;
    AudioInfo.pitch = pitch;
    AudioInfo.bIs3D = true;
    AudioInfo.bUseSoundsPitchVal = false;
    AudioInfo.iSound = iSound + eSFX_MAX;
#if defined(_DEBUG)
    strncpy(AudioInfo.chName, (char*)szSoundName, 64);
#endif

    S32 token = AIL_enqueue_event_start();
    AIL_enqueue_event_buffer(&token, &AudioInfo, sizeof(AUDIO_INFO), 0);
    AIL_enqueue_event_end_named(token, (char*)szSoundName);
}

/////////////////////////////////////////////
//
//	playUI
//
/////////////////////////////////////////////
void SoundEngine::playUI(int iSound, float volume, float pitch) {
    U8 szSoundName[256];
    std::string name;
    // we have some game sounds played as UI sounds...
    // Not the best way to do this, but it seems to only be the portal sounds

    if (iSound >= eSFX_MAX) {
        // AP removed old counting system. Now relying on Miles' Play Count
        // Limit
        /*		// if we are already playing loads of this sounds ignore
        this one
        if(CurrentSoundsPlaying[iSound+eSFX_MAX]>MAX_SAME_SOUNDS_PLAYING)
        return;*/

        // build the name
        strcpy((char*)szSoundName, "Minecraft/");
        name = wchSoundNames[iSound];
    } else {
        // AP removed old counting system. Now relying on Miles' Play Count
        // Limit
        /*		// if we are already playing loads of this sounds ignore
        this one if(CurrentSoundsPlaying[iSound]>MAX_SAME_SOUNDS_PLAYING)
        return;*/

        // build the name
        strcpy((char*)szSoundName, "Minecraft/UI/");
        name = wchUISoundNames[iSound];
    }

    char* SoundName = (char*)ConvertSoundPathToName(name);
    strcat((char*)szSoundName, SoundName);
    //	app.DebugPrintf("UI: Playing %s, volume %f, pitch
    //%f\n",SoundName,volume,pitch);

    // app.DebugPrintf("PlaySound - %d - %s\n",iSound, SoundName);

    AUDIO_INFO AudioInfo;
    memset(&AudioInfo, 0, sizeof(AUDIO_INFO));
    AudioInfo.volume = volume;  // will be multiplied by the master volume
    AudioInfo.pitch = pitch;
    AudioInfo.bUseSoundsPitchVal = true;
    if (iSound >= eSFX_MAX) {
        AudioInfo.iSound = iSound + eSFX_MAX;
    } else {
        AudioInfo.iSound = iSound;
    }
#if defined(_DEBUG)
    strncpy(AudioInfo.chName, (char*)szSoundName, 64);
#endif

    // 4J-PB - not going to stop UI events happening based on the number of
    // currently playing sounds
    S32 token = AIL_enqueue_event_start();
    AIL_enqueue_event_buffer(&token, &AudioInfo, sizeof(AUDIO_INFO), 0);
    AIL_enqueue_event_end_named(token, (char*)szSoundName);
}
/////////////////////////////////////////////
//
//	playStreaming
//
/////////////////////////////////////////////
void SoundEngine::playStreaming(const std::string& name, float x, float y,
                                float z, float volume, float pitch,
                                bool bMusicDelay) {
    // This function doesn't actually play a streaming sound, just sets states
    // and an id for the music tick to play it Level audio will be played when a
    // play with an empty name comes in CD audio will be played when a named
    // stream comes in

    m_StreamingAudioInfo.x = x;
    m_StreamingAudioInfo.y = y;
    m_StreamingAudioInfo.z = z;
    m_StreamingAudioInfo.volume = volume;
    m_StreamingAudioInfo.pitch = pitch;

    if (m_StreamState == eMusicStreamState_Playing) {
        m_StreamState = eMusicStreamState_Stop;
    } else if (m_StreamState == eMusicStreamState_Opening) {
        m_StreamState = eMusicStreamState_OpeningCancel;
    }

    if (name.empty()) {
        // music, or stop CD
        m_StreamingAudioInfo.bIs3D = false;

        // we need a music id
        // random delay of up to 3 minutes for music
        m_iMusicDelay = random->nextInt(
            20 * 60 * 3);  // random->nextInt(20 * 60 * 10) + 20 * 60 * 10;

#if defined(_DEBUG)
        m_iMusicDelay = 0;
#endif
        Minecraft* pMinecraft = Minecraft::GetInstance();

        bool playerInEnd = false;
        bool playerInNether = false;

        for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
            if (pMinecraft->localplayers[i] != nullptr) {
                if (pMinecraft->localplayers[i]->dimension ==
                    LevelData::DIMENSION_END) {
                    playerInEnd = true;
                } else if (pMinecraft->localplayers[i]->dimension ==
                           LevelData::DIMENSION_NETHER) {
                    playerInNether = true;
                }
            }
        }
        if (playerInEnd) {
            m_musicID = getMusicID(LevelData::DIMENSION_END);
        } else if (playerInNether) {
            m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
        } else {
            m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
        }
    } else {
        // jukebox
        m_StreamingAudioInfo.bIs3D = true;
        m_musicID = getMusicID(name);
        m_iMusicDelay = 0;
    }
}
int SoundEngine::OpenStreamThreadProc(void* lpParameter) {
#if defined(__DISABLE_MILES__)
    return 0;
#endif
    SoundEngine* soundEngine = (SoundEngine*)lpParameter;
    soundEngine->m_hStream =
        AIL_open_stream(soundEngine->m_hDriver, soundEngine->m_szStreamName, 0);
    return 0;
}
/////////////////////////////////////////////
//
//	playMusicTick
//
/////////////////////////////////////////////
void SoundEngine::playMusicTick() {
    // AP - vita will update the music during the mixer callback
    playMusicUpdate();
}

// AP - moved to a separate function so it can be called from the mixer callback
// on Vita
void SoundEngine::playMusicUpdate() {
    // return;
    static bool firstCall = true;
    static float fMusicVol = 0.0f;
    if (firstCall) {
        fMusicVol = getMasterMusicVolume();
        firstCall = false;
    }

    switch (m_StreamState) {
        case eMusicStreamState_Idle:

            // start a stream playing
            if (m_iMusicDelay > 0) {
                m_iMusicDelay--;
                return;
            }

            if (m_musicID != -1) {
                // start playing it

                strcpy((char*)m_szStreamName, m_szMusicPath);
                // are we using a mash-up pack?
                // if(pMinecraft && !pMinecraft->skins->isUsingDefaultSkin() &&
                // pMinecraft->skins->getSelected()->hasAudio())
                if (Minecraft::GetInstance()
                        ->skins->getSelected()
                        ->hasAudio()) {
                    // It's a mash-up - need to use the DLC path for the music
                    TexturePack* pTexPack =
                        Minecraft::GetInstance()->skins->getSelected();
                    DLCTexturePack* pDLCTexPack = (DLCTexturePack*)pTexPack;
                    DLCPack* pack = pDLCTexPack->getDLCInfoParentPack();
                    DLCAudioFile* dlcAudioFile = (DLCAudioFile*)pack->getFile(
                        DLCManager::e_DLCType_Audio, 0);

                    app.DebugPrintf("Mashup pack \n");

                    // build the name

                    // if the music ID is beyond the end of the texture pack
                    // music files, then it's a CD
                    if (m_musicID < m_iStream_CD_1) {
                        SetIsPlayingStreamingGameMusic(true);
                        SetIsPlayingStreamingCDMusic(false);
                        m_MusicType = eMusicType_Game;
                        m_StreamingAudioInfo.bIs3D = false;

                        std::string& wstrSoundName =
                            dlcAudioFile->GetSoundName(m_musicID);
                        char szName[255];
                        strncpy(szName, wstrSoundName.c_str(), 255);

                        std::string strFile =
                            "TPACK:\\Data\\" + string(szName) + ".binka";
                        std::string mountedPath =
                            PlatformStorage.GetMountedPath(strFile);
                        strcpy(m_szStreamName, mountedPath.c_str());
                    } else {
                        SetIsPlayingStreamingGameMusic(false);
                        SetIsPlayingStreamingCDMusic(true);
                        m_MusicType = eMusicType_CD;
                        m_StreamingAudioInfo.bIs3D = true;

                        // Need to adjust to index into the cds in the game's
                        // m_szStreamFileA
                        strcat((char*)m_szStreamName, "cds/");
                        strcat((char*)m_szStreamName,
                               m_szStreamFileA[m_musicID - m_iStream_CD_1 +
                                               eStream_CD_1]);
                        strcat((char*)m_szStreamName, ".binka");
                    }
                } else {
                    // 4J-PB - if this is a PS3 disc patch, we have to check if
                    // the music file is in the patch data
                    if (m_musicID < m_iStream_CD_1) {
                        SetIsPlayingStreamingGameMusic(true);
                        SetIsPlayingStreamingCDMusic(false);
                        m_MusicType = eMusicType_Game;
                        m_StreamingAudioInfo.bIs3D = false;
                        // build the name
                        strcat((char*)m_szStreamName, "music/");
                    } else {
                        SetIsPlayingStreamingGameMusic(false);
                        SetIsPlayingStreamingCDMusic(true);
                        m_MusicType = eMusicType_CD;
                        m_StreamingAudioInfo.bIs3D = true;
                        // build the name
                        strcat((char*)m_szStreamName, "cds/");
                    }
                    strcat((char*)m_szStreamName, m_szStreamFileA[m_musicID]);
                    strcat((char*)m_szStreamName, ".binka");
                }

                // std::string name =
                // m_szStreamFileA[m_musicID];char*SoundName=(char
                // *)ConvertSoundPathToName(name);strcat((char
                // *)szStreamName,SoundName);

                app.DebugPrintf("Starting streaming - %s\n", m_szStreamName);

                // Don't actually open in this thread, as it can block for
                // ~300ms.
                m_openStreamThread = new C4JThread(OpenStreamThreadProc, this,
                                                   "OpenStreamThreadProc");
                m_openStreamThread->run();
                m_StreamState = eMusicStreamState_Opening;
            }
            break;

        case eMusicStreamState_Opening:
            // If the open stream thread is complete, then we are ready to
            // proceed to actually playing
            if (!m_openStreamThread->isRunning()) {
                delete m_openStreamThread;
                m_openStreamThread = nullptr;

                HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);

                // 4J-PB - causes the falloff to be calculated on the PPU
                // instead of the SPU, and seems to resolve our distorted sound
                // issue
                AIL_register_falloff_function_callback(
                    hSample, &custom_falloff_function);

                if (m_StreamingAudioInfo.bIs3D) {
                    AIL_set_sample_3D_distances(
                        hSample, 64.0f, 1,
                        0);  // Larger distance scaler for music discs
                    if (m_validListenerCount > 1) {
                        float fClosest = 10000.0f;
                        int iClosestListener = 0;
                        float fClosestX = 0.0f, fClosestY = 0.0f,
                              fClosestZ = 0.0f, fDist;
                        // need to calculate the distance from the sound to the
                        // nearest listener - use Manhattan Distance as the
                        // decision
                        for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                            if (m_ListenerA[i].bValid) {
                                float x, y, z;

                                x = fabs(m_ListenerA[i].vPosition.x -
                                         m_StreamingAudioInfo.x);
                                y = fabs(m_ListenerA[i].vPosition.y -
                                         m_StreamingAudioInfo.y);
                                z = fabs(m_ListenerA[i].vPosition.z -
                                         m_StreamingAudioInfo.z);
                                fDist = x + y + z;

                                if (fDist < fClosest) {
                                    fClosest = fDist;
                                    fClosestX = x;
                                    fClosestY = y;
                                    fClosestZ = z;
                                    iClosestListener = i;
                                }
                            }
                        }

                        // our distances in the world aren't very big, so floats
                        // rather than casts to doubles should be fine
                        fDist = sqrtf((fClosestX * fClosestX) +
                                      (fClosestY * fClosestY) +
                                      (fClosestZ * fClosestZ));
                        AIL_set_sample_3D_position(hSample, 0, 0, fDist);
                    } else {
                        AIL_set_sample_3D_position(
                            hSample, m_StreamingAudioInfo.x,
                            m_StreamingAudioInfo.y,
                            -m_StreamingAudioInfo
                                 .z);  // Flipped sign of z as Miles is
                                       // expecting left handed coord system
                    }
                } else {
                    // clear the 3d flag on the stream after a jukebox finishes
                    // and streaming music starts
                    AIL_set_sample_is_3D(hSample, 0);
                }
                // set the pitch
                app.DebugPrintf("Sample rate:%d\n",
                                AIL_sample_playback_rate(hSample));
                AIL_set_sample_playback_rate_factor(hSample,
                                                    m_StreamingAudioInfo.pitch);
                // set the volume
                AIL_set_sample_volume_levels(
                    hSample,
                    m_StreamingAudioInfo.volume * getMasterMusicVolume(),
                    m_StreamingAudioInfo.volume * getMasterMusicVolume());

                AIL_start_stream(m_hStream);

                m_StreamState = eMusicStreamState_Playing;
            }
            break;
        case eMusicStreamState_OpeningCancel:
            if (!m_openStreamThread->isRunning()) {
                delete m_openStreamThread;
                m_openStreamThread = nullptr;
                m_StreamState = eMusicStreamState_Stop;
            }
            break;
        case eMusicStreamState_Stop:
            // should gradually take the volume down in steps
            AIL_pause_stream(m_hStream, 1);
            AIL_close_stream(m_hStream);
            m_hStream = 0;
            SetIsPlayingStreamingCDMusic(false);
            SetIsPlayingStreamingGameMusic(false);
            m_StreamState = eMusicStreamState_Idle;
            break;
        case eMusicStreamState_Stopping:
            break;
        case eMusicStreamState_Play:
            break;
        case eMusicStreamState_Playing:
            if (GetIsPlayingStreamingGameMusic()) {
                // if(m_MusicInfo.pCue!=nullptr)
                {
                    bool playerInEnd = false;
                    bool playerInNether = false;
                    Minecraft* pMinecraft = Minecraft::GetInstance();
                    for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; ++i) {
                        if (pMinecraft->localplayers[i] != nullptr) {
                            if (pMinecraft->localplayers[i]->dimension ==
                                LevelData::DIMENSION_END) {
                                playerInEnd = true;
                            } else if (pMinecraft->localplayers[i]->dimension ==
                                       LevelData::DIMENSION_NETHER) {
                                playerInNether = true;
                            }
                        }
                    }

                    if (playerInEnd && !GetIsPlayingEndMusic()) {
                        m_StreamState = eMusicStreamState_Stop;

                        // Set the end track
                        m_musicID = getMusicID(LevelData::DIMENSION_END);
                        SetIsPlayingEndMusic(true);
                        SetIsPlayingNetherMusic(false);
                    } else if (!playerInEnd && GetIsPlayingEndMusic()) {
                        if (playerInNether) {
                            m_StreamState = eMusicStreamState_Stop;

                            // Set the end track
                            m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
                            SetIsPlayingEndMusic(false);
                            SetIsPlayingNetherMusic(true);
                        } else {
                            m_StreamState = eMusicStreamState_Stop;

                            // Set the end track
                            m_musicID =
                                getMusicID(LevelData::DIMENSION_OVERWORLD);
                            SetIsPlayingEndMusic(false);
                            SetIsPlayingNetherMusic(false);
                        }
                    } else if (playerInNether && !GetIsPlayingNetherMusic()) {
                        m_StreamState = eMusicStreamState_Stop;
                        // set the Nether track
                        m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
                        SetIsPlayingNetherMusic(true);
                        SetIsPlayingEndMusic(false);
                    } else if (!playerInNether && GetIsPlayingNetherMusic()) {
                        if (playerInEnd) {
                            m_StreamState = eMusicStreamState_Stop;
                            // set the Nether track
                            m_musicID = getMusicID(LevelData::DIMENSION_END);
                            SetIsPlayingNetherMusic(false);
                            SetIsPlayingEndMusic(true);
                        } else {
                            m_StreamState = eMusicStreamState_Stop;
                            // set the Nether track
                            m_musicID =
                                getMusicID(LevelData::DIMENSION_OVERWORLD);
                            SetIsPlayingNetherMusic(false);
                            SetIsPlayingEndMusic(false);
                        }
                    }

                    // volume change required?
                    if (fMusicVol != getMasterMusicVolume()) {
                        fMusicVol = getMasterMusicVolume();
                        HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);
                        // AIL_set_sample_3D_position( hSample,
                        // m_StreamingAudioInfo.x, m_StreamingAudioInfo.y,
                        // m_StreamingAudioInfo.z );
                        AIL_set_sample_volume_levels(hSample, fMusicVol,
                                                     fMusicVol);
                    }
                }
            } else {
                // Music disc playing - if it's a 3D stream, then set the
                // position - we don't have any streaming audio in the world
                // that moves, so this isn't required unless we have more than
                // one listener, and are setting the listening position to the
                // origin and setting a fake position for the sound down  the z
                // axis
                if (m_StreamingAudioInfo.bIs3D) {
                    if (m_validListenerCount > 1) {
                        float fClosest = 10000.0f;
                        int iClosestListener = 0;
                        float fClosestX = 0.0f, fClosestY = 0.0f,
                              fClosestZ = 0.0f, fDist;

                        // need to calculate the distance from the sound to the
                        // nearest listener - use Manhattan Distance as the
                        // decision
                        for (int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                            if (m_ListenerA[i].bValid) {
                                float x, y, z;

                                x = fabs(m_ListenerA[i].vPosition.x -
                                         m_StreamingAudioInfo.x);
                                y = fabs(m_ListenerA[i].vPosition.y -
                                         m_StreamingAudioInfo.y);
                                z = fabs(m_ListenerA[i].vPosition.z -
                                         m_StreamingAudioInfo.z);
                                fDist = x + y + z;

                                if (fDist < fClosest) {
                                    fClosest = fDist;
                                    fClosestX = x;
                                    fClosestY = y;
                                    fClosestZ = z;
                                    iClosestListener = i;
                                }
                            }
                        }

                        // our distances in the world aren't very big, so floats
                        // rather than casts to doubles should be fine
                        HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);
                        fDist = sqrtf((fClosestX * fClosestX) +
                                      (fClosestY * fClosestY) +
                                      (fClosestZ * fClosestZ));
                        AIL_set_sample_3D_position(hSample, 0, 0, fDist);
                    }
                }
            }

            break;

        case eMusicStreamState_Completed: {
            // random delay of up to 3 minutes for music
            m_iMusicDelay = random->nextInt(
                20 * 60 * 3);  // random->nextInt(20 * 60 * 10) + 20 * 60 * 10;
            // Check if we have a local player in The Nether or in The End, and
            // play that music if they are
            Minecraft* pMinecraft = Minecraft::GetInstance();
            bool playerInEnd = false;
            bool playerInNether = false;

            for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; i++) {
                if (pMinecraft->localplayers[i] != nullptr) {
                    if (pMinecraft->localplayers[i]->dimension ==
                        LevelData::DIMENSION_END) {
                        playerInEnd = true;
                    } else if (pMinecraft->localplayers[i]->dimension ==
                               LevelData::DIMENSION_NETHER) {
                        playerInNether = true;
                    }
                }
            }
            if (playerInEnd) {
                m_musicID = getMusicID(LevelData::DIMENSION_END);
                SetIsPlayingEndMusic(true);
                SetIsPlayingNetherMusic(false);
            } else if (playerInNether) {
                m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
                SetIsPlayingNetherMusic(true);
                SetIsPlayingEndMusic(false);
            } else {
                m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
                SetIsPlayingNetherMusic(false);
                SetIsPlayingEndMusic(false);
            }

            m_StreamState = eMusicStreamState_Idle;
        } break;
    }

    // check the status of the stream - this is for when a track completes
    // rather than is stopped by the user action

    if (m_hStream != 0) {
        if (AIL_stream_status(m_hStream) == SMP_DONE)  // SMP_DONE
        {
            AIL_close_stream(m_hStream);
            m_hStream = 0;
            SetIsPlayingStreamingCDMusic(false);
            SetIsPlayingStreamingGameMusic(false);

            m_StreamState = eMusicStreamState_Completed;
        }
    }
}
F32 AILCALLBACK custom_falloff_function(HSAMPLE S, F32 distance,
                                        F32 rolloff_factor, F32 min_dist,
                                        F32 max_dist) {
    F32 result;

    // This is now emulating the linear fall-off function that we used on the
    // Xbox 360. The parameter which is passed as "max_dist" is the only one
    // actually used, and is generally used as CurveDistanceScaler is used on
    // XACT on the Xbox. A special value of 10000.0f is passed for thunder,
    // which has no attenuation

    if (max_dist == 10000.0f) {
        return 1.0f;
    }

    result = 1.0f - (distance / max_dist);
    if (result < 0.0f) result = 0.0f;
    if (result > 1.0f) result = 1.0f;

    return result;
}
#endif

// Universal, these functions shouldn't need platform specific
// implementations
void SoundEngine::updateMusicVolume(float fVal) { m_MasterMusicVolume = fVal; }
void SoundEngine::updateSystemMusicPlaying(bool isPlaying) {
    m_bSystemMusicPlaying = isPlaying;
}
void SoundEngine::updateSoundEffectVolume(float fVal) {
    m_MasterEffectsVolume = fVal;
}
void SoundEngine::SetStreamingSounds(int iOverworldMin, int iOverWorldMax,
                                     int iNetherMin, int iNetherMax,
                                     int iEndMin, int iEndMax, int iCD1) {
    m_iStream_Overworld_Min = iOverworldMin;
    m_iStream_Overworld_Max = iOverWorldMax;
    m_iStream_Nether_Min = iNetherMin;
    m_iStream_Nether_Max = iNetherMax;
    m_iStream_End_Min = iEndMin;
    m_iStream_End_Max = iEndMax;
    m_iStream_CD_1 = iCD1;

    // array to monitor recently played tracks
    if (m_bHeardTrackA) {
        delete[] m_bHeardTrackA;
    }
    m_bHeardTrackA = new bool[iEndMax + 1];
    memset(m_bHeardTrackA, 0, sizeof(bool) * (iEndMax + 1));
}
int SoundEngine::GetRandomishTrack(int iStart, int iEnd) {
    // 4J-PB - make it more likely that we'll get a track we've not heard for a
    // while, although repeating tracks sometimes is fine

    // if all tracks have been heard, clear the flags
    bool bAllTracksHeard = true;
    int iVal = iStart;
    for (size_t i = iStart; i <= iEnd; i++) {
        if (m_bHeardTrackA[i] == false) {
            bAllTracksHeard = false;
            app.DebugPrintf("Not heard all tracks yet\n");
            break;
        }
    }

    if (bAllTracksHeard) {
        app.DebugPrintf("Heard all tracks - resetting the tracking array\n");

        for (size_t i = iStart; i <= iEnd; i++) {
            m_bHeardTrackA[i] = false;
        }
    }

    // trying to get a track we haven't heard, but not too hard
    for (size_t i = 0; i <= ((iEnd - iStart) / 2); i++) {
        // random->nextInt(1) will always return 0
        iVal = random->nextInt((iEnd - iStart) + 1) + iStart;
        if (m_bHeardTrackA[iVal] == false) {
            // not heard this
            app.DebugPrintf("(%d) Not heard track %d yet, so playing it now\n",
                            i, iVal);
            m_bHeardTrackA[iVal] = true;
            break;
        } else {
            app.DebugPrintf(
                "(%d) Skipping track %d already heard it recently\n", i, iVal);
        }
    }

    app.DebugPrintf("Select track %d\n", iVal);
    return iVal;
}
float SoundEngine::getMasterMusicVolume() {
    if (m_bSystemMusicPlaying) {
        return 0.0f;
    } else {
        return m_MasterMusicVolume;
    }
}
void SoundEngine::add(const std::string& name, File* file) {}

void SoundEngine::addMusic(const std::string& name, File* file) {}
void SoundEngine::addStreaming(const std::string& name, File* file) {}

bool SoundEngine::isStreamingWavebankReady() { return true; }
// This is unused by the linux version, it'll need to be changed
char* SoundEngine::ConvertSoundPathToName(const std::string& name,
                                          bool bConvertSpaces) {
    return nullptr;
}

void ConsoleSoundEngine::tick() {
    if (scheduledSounds.empty()) {
        return;
    }

    for (auto it = scheduledSounds.begin(); it != scheduledSounds.end();) {
        SoundEngine::ScheduledSound* next = *it;
        next->delay--;

        if (next->delay <= 0) {
            play(next->iSound, next->x, next->y, next->z, next->volume,
                 next->pitch);
            it = scheduledSounds.erase(it);
            delete next;
        } else {
            ++it;
        }
    }
}

void ConsoleSoundEngine::schedule(int iSound, float x, float y, float z,
                                  float volume, float pitch, int delayTicks) {
    scheduledSounds.push_back(new SoundEngine::ScheduledSound(
        iSound, x, y, z, volume, pitch, delayTicks));
}

ConsoleSoundEngine::ScheduledSound::ScheduledSound(int iSound, float x, float y,
                                                   float z, float volume,
                                                   float pitch, int delay) {
    this->iSound = iSound;
    this->x = x;
    this->y = y;
    this->z = z;
    this->volume = volume;
    this->pitch = pitch;
    this->delay = delay;
}
