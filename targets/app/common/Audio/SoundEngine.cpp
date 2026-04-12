#include "SoundEngine.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <vector>

#include "app/common/Audio/ConsoleSoundEngine.h"
#include "app/common/Game.h"
#include "app/common/Iggy/include/rrCore.h"
#include "java/Random.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/util/Mth.h"
#include "minecraft/world/entity/Mob.h"
#include "minecraft/world/level/storage/LevelData.h"
#include "platform/PlatformTypes.h"
#include "platform/fs/fs.h"
#include "platform/thread/C4JThread.h"

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
#if defined(__APPLE__)
#include <miniaudio/miniaudio.h>
#else
#include "miniaudio.h"
#endif

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

// stb_vorbis leaks single-letter macros (C, L, R, etc.) that collide with
// identifiers in other translation units during unity builds.
#undef C
#undef L
#undef R
#undef TRUE
#undef FALSE

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
char SoundEngine::m_szSoundPath[] = {"app/common/Sound/"};
char SoundEngine::m_szMusicPath[] = {"app/common/"};
char SoundEngine::m_szRedistName[] = {"redist64"};

// END ASSETS

// Linux specific functions

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

SoundEngine::SoundEngine()
    : m_audio(std::make_unique<SoundEngineMiniAudio>()) {}
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

    if (ma_engine_init(&m_audio->engineConfig, &m_audio->engine) !=
        MA_SUCCESS) {
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
    const char* roots[] = {"Sound/Minecraft/", "app/common/Sound/Minecraft/",
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

    if (ma_sound_init_from_file(&m_audio->engine, finalPath,
                                MA_SOUND_FLAG_ASYNC, nullptr, nullptr,
                                &s->sound) == MA_SUCCESS) {
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

    if (ma_sound_init_from_file(&m_audio->engine, finalPath,
                                MA_SOUND_FLAG_ASYNC, nullptr, nullptr,
                                &s->sound) == MA_SUCCESS) {
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
        MA_SOUND_FLAG_STREAM, nullptr, nullptr,
        &soundEngine->m_audio->musicStream);

    if (result != MA_SUCCESS) {
        app.DebugPrintf(
            "SoundEngine::OpenStreamThreadProc - Failed to open stream: "
            "%s\n",
            soundEngine->m_szStreamName);
        return 0;
    }

    ma_sound_set_spatialization_enabled(&soundEngine->m_audio->musicStream,
                                        MA_FALSE);
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
                std::string base =
                    PlatformFilesystem.getBasePath().string() + "/";
                bool isCD = (m_musicID >= m_iStream_CD_1);
                const char* folder = isCD ? "cds/" : "music/";
                const char* track = m_szStreamFileA[m_musicID];
                bool found = false;
                m_szStreamName[0] = '\0';

                const char* roots[] = {"app/common/music/", "music/", "./"};

                for (const char* r : roots) {
                    for (const char* e : {".ogg", ".mp3", ".wav"}) {
                        // try with folder prefix (music/ or cds/)
                        snprintf(m_szStreamName, sizeof(m_szStreamName),
                                 "%s%s%s%s%s", base.c_str(), r, folder, track,
                                 e);
                        if (PlatformFilesystem.exists(m_szStreamName)) {
                            found = true;
                            break;
                        }
                        // try without folder prefix
                        snprintf(m_szStreamName, sizeof(m_szStreamName),
                                 "%s%s%s%s", base.c_str(), r, track, e);
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

                ma_sound_set_pitch(&m_audio->musicStream,
                                   m_StreamingAudioInfo.pitch);
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

                ma_engine_listener_set_world_up(&m_audio->engine, 0, 0.0f, 1.0f,
                                                0.0f);

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
