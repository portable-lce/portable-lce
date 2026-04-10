#pragma once
class Mob;
class Options;
class C4JThread;
class Random;

#include <memory>
#include <string>

#include "app/common/Audio/ConsoleSoundEngine.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Audio/SoundTypes.h"
#include "platform/PlatformTypes.h"

// Forward-declare the miniaudio backing state. The full struct lives in
// SoundEngine.cpp where miniaudio.h is actually included. This keeps
// miniaudio.h out of the 9 minecraft files that include SoundEngine.h
// via the 4j include chain.
struct SoundEngineMiniAudio;

constexpr float SFX_3D_MIN_DISTANCE = 1.0f;
constexpr float SFX_3D_MAX_DISTANCE = 16.0f;
constexpr float SFX_3D_ROLLOFF = 0.5f;
constexpr float SFX_VOLUME_MULTIPLIER = 1.5f;
constexpr float SFX_MAX_GAIN = 1.5f;
enum eMUSICFILES {
    eStream_Overworld_Calm1 = 0,
    eStream_Overworld_Calm2,
    eStream_Overworld_Calm3,
    eStream_Overworld_hal1,
    eStream_Overworld_hal2,
    eStream_Overworld_hal3,
    eStream_Overworld_hal4,
    eStream_Overworld_nuance1,
    eStream_Overworld_nuance2,
    // Add the new music tracks
    eStream_Overworld_Creative1,
    eStream_Overworld_Creative2,
    eStream_Overworld_Creative3,
    eStream_Overworld_Creative4,
    eStream_Overworld_Creative5,
    eStream_Overworld_Creative6,
    eStream_Overworld_Menu1,
    eStream_Overworld_Menu2,
    eStream_Overworld_Menu3,
    eStream_Overworld_Menu4,
    eStream_Overworld_piano1,
    eStream_Overworld_piano2,
    eStream_Overworld_piano3,  // <-- make piano3 the last overworld one
    // Nether
    eStream_Nether1,
    eStream_Nether2,
    eStream_Nether3,
    eStream_Nether4,
    // The End
    eStream_end_dragon,
    eStream_end_end,
    eStream_CD_1,
    eStream_CD_2,
    eStream_CD_3,
    eStream_CD_4,
    eStream_CD_5,
    eStream_CD_6,
    eStream_CD_7,
    eStream_CD_8,
    eStream_CD_9,
    eStream_CD_10,
    eStream_CD_11,
    eStream_CD_12,
    eStream_Max,
};

enum eMUSICTYPE {
    eMusicType_None,
    eMusicType_Game,
    eMusicType_CD,
};

enum MUSIC_STREAMSTATE {
    eMusicStreamState_Idle = 0,
    eMusicStreamState_Stop,
    eMusicStreamState_Stopping,
    eMusicStreamState_Opening,
    eMusicStreamState_OpeningCancel,
    eMusicStreamState_Play,
    eMusicStreamState_Playing,
    eMusicStreamState_Completed
};

typedef struct {
    F32 x, y, z, volume, pitch;
    int iSound;
    bool bIs3D;
    bool bUseSoundsPitchVal;
#if defined(_DEBUG)
    char chName[64];
#endif
} AUDIO_INFO;

// MiniAudioSound's definition lives in SoundEngine.cpp alongside the
// miniaudio backend; consumers of this header don't need to see it.

class SoundEngine : public ConsoleSoundEngine {
    static const int MAX_SAME_SOUNDS_PLAYING = 8;  // 4J added
public:
    SoundEngine();
    ~SoundEngine();
    virtual void destroy();
#if defined(_DEBUG)
    void GetSoundName(char* szSoundName, int iSound);
#endif
    virtual void play(int iSound, float x, float y, float z, float volume,
                      float pitch);
    virtual void playStreaming(const std::string& name, float x, float y,
                               float z, float volume, float pitch,
                               bool bMusicDelay = true);
    virtual void playUI(int iSound, float volume, float pitch);
    virtual void playMusicTick();
    virtual void updateMusicVolume(float fVal);
    virtual void updateSystemMusicPlaying(bool isPlaying);
    virtual void updateSoundEffectVolume(float fVal);
    virtual void init(Options*);
    virtual void tick(std::shared_ptr<Mob>* players,
                      float a);  // 4J - updated to take array of local players
                                 // rather than single one
    virtual void add(const std::string& name, File* file);
    virtual void addMusic(const std::string& name, File* file);
    virtual void addStreaming(const std::string& name, File* file);
    virtual char* ConvertSoundPathToName(const std::string& name,
                                         bool bConvertSpaces = false);
    bool isStreamingWavebankReady();  // 4J Added
    int getMusicID(int iDomain);
    int getMusicID(const std::string& name);
    void SetStreamingSounds(int iOverworldMin, int iOverWorldMax,
                            int iNetherMin, int iNetherMax, int iEndMin,
                            int iEndMax, int iCD1);
    void updateMiles();  // AP added so Vita can update all the Miles functions
                         // during the mixer callback
    void playMusicUpdate();

private:
    float getMasterMusicVolume();
    // platform specific functions
    int initAudioHardware(int iMinSpeakers) { return iMinSpeakers; }
#if defined(__linux__)
    void updateMiniAudio();
#endif

    int GetRandomishTrack(int iStart, int iEnd);

    // Miniaudio engine + music stream PIMPL'd into SoundEngineMiniAudio,
    // defined in SoundEngine.cpp. Owned via unique_ptr; the destructor
    // is out-of-line so the unique_ptr instantiation can see the full
    // type.
    std::unique_ptr<SoundEngineMiniAudio> m_audio;
    bool m_musicStreamActive;

    static char m_szSoundPath[];
    static char m_szMusicPath[];
    static char m_szRedistName[];
    static const char* m_szStreamFileA[eStream_Max];

    AUDIO_LISTENER m_ListenerA[MAX_LOCAL_PLAYERS];
    int m_validListenerCount;

    Random* random;
    int m_musicID;
    int m_iMusicDelay;
    int m_StreamState;
    int m_MusicType;
    AUDIO_INFO m_StreamingAudioInfo;
    std::string m_CDMusic;
    bool m_bSystemMusicPlaying;
    float m_MasterMusicVolume;
    float m_MasterEffectsVolume;

    C4JThread* m_openStreamThread;
    static int OpenStreamThreadProc(void* lpParameter);
    char m_szStreamName[1024];
    int CurrentSoundsPlaying[static_cast<int>(eSoundType_MAX) +
                             static_cast<int>(eSFX_MAX)];

    // streaming music files - will be different for mash-up packs
    int m_iStream_Overworld_Min, m_iStream_Overworld_Max;
    int m_iStream_Nether_Min, m_iStream_Nether_Max;
    int m_iStream_End_Min, m_iStream_End_Max;
    int m_iStream_CD_1;
    bool* m_bHeardTrackA;
};
