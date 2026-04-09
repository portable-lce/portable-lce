#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SoundTypes.h"

class File;

typedef struct {
    float x, y, z;
} AUDIO_VECTOR;

typedef struct {
    bool bValid;
    AUDIO_VECTOR vPosition;
    AUDIO_VECTOR vOrientFront;
} AUDIO_LISTENER;

class Options;
class Mob;

// Game-side sound engine interface. The concrete backend
// (app/common/Audio/SoundEngine) inherits from this and forwards into
// IPlatformSound. minecraft/ consumers see only the abstract base, so
// they don't have to drag the concrete backend (and its miniaudio
// pimpl) into their compilation units.
class ConsoleSoundEngine {
public:
    ConsoleSoundEngine()
        : m_bIsPlayingStreamingCDMusic(false),
          m_bIsPlayingStreamingGameMusic(false),
          m_bIsPlayingEndMusic(false),
          m_bIsPlayingNetherMusic(false) {}

    virtual ~ConsoleSoundEngine() = default;

    virtual void tick(std::shared_ptr<Mob>* players, float a) = 0;
    virtual void destroy() = 0;
    virtual void play(int iSound, float x, float y, float z, float volume,
                      float pitch) = 0;
    virtual void playStreaming(const std::string& name, float x, float y,
                               float z, float volume, float pitch,
                               bool bMusicDelay = true) = 0;
    virtual void playUI(int iSound, float volume, float pitch) = 0;
    virtual void updateMusicVolume(float fVal) = 0;
    virtual void updateSystemMusicPlaying(bool isPlaying) = 0;
    virtual void updateSoundEffectVolume(float fVal) = 0;
    virtual void init(Options*) = 0;
    virtual void add(const std::string& name, File* file) = 0;
    virtual void addMusic(const std::string& name, File* file) = 0;
    virtual void addStreaming(const std::string& name, File* file) = 0;
    virtual char* ConvertSoundPathToName(const std::string& name,
                                         bool bConvertSpaces) = 0;
    virtual void playMusicTick() = 0;

    virtual bool GetIsPlayingStreamingCDMusic();
    virtual bool GetIsPlayingStreamingGameMusic();
    virtual void SetIsPlayingStreamingCDMusic(bool bVal);
    virtual void SetIsPlayingStreamingGameMusic(bool bVal);
    virtual bool GetIsPlayingEndMusic();
    virtual bool GetIsPlayingNetherMusic();
    virtual void SetIsPlayingEndMusic(bool bVal);
    virtual void SetIsPlayingNetherMusic(bool bVal);

    static const char* wchSoundNames[eSoundType_MAX];
    static const char* wchUISoundNames[eSFX_MAX];

public:
    void tick();
    void schedule(int iSound, float x, float y, float z, float volume,
                  float pitch, int delayTicks);

private:
    class ScheduledSound {
    public:
        int iSound;
        float x, y, z;
        float volume, pitch;
        int delay;

    public:
        ScheduledSound(int iSound, float x, float y, float z, float volume,
                       float pitch, int delay);
    };

    std::vector<ScheduledSound*> scheduledSounds;

    virtual int initAudioHardware(int iMinSpeakers) = 0;

    bool m_bIsPlayingStreamingCDMusic;
    bool m_bIsPlayingStreamingGameMusic;
    bool m_bIsPlayingEndMusic;
    bool m_bIsPlayingNetherMusic;
};
