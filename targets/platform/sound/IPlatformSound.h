#pragma once

#include <cstdint>
#include <string>

#include "platform/sound/SoundHandles.h"

// Platform sound interface. The backend (currently miniaudio) implements
// this; consumers in app/common/Audio/SoundEngine talk to the interface
// rather than to miniaudio directly.
//
// The interface is intent-level: load a sample from disk, play it,
// stop it, set listener position. State (volume, pitch, looping) is
// passed at play time as a struct rather than via global setters.
//
// Concrete handles (SoundHandle, MusicHandle) are tagged structs so the
// type system catches sample-vs-music confusion at compile time.

namespace platform::sound {

struct PlaySoundParams {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool spatial = true;       // 3D sound positioned in the world
    bool looping = false;
    float minDistance = 1.0f;  // distance below which the sound is full-volume
    float maxDistance = 16.0f; // distance above which the sound is silent
};

struct PlayMusicParams {
    float volume = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
};

class IPlatformSound {
public:
    virtual ~IPlatformSound() = default;

    // Lifecycle. init() takes the maximum number of simultaneous local
    // listeners (splitscreen player count). Idempotent: a second init()
    // call with the same parameters is a no-op.
    virtual void init(int listenerCount) = 0;
    virtual void shutdown() = 0;

    // Per-frame tick. Drives streaming reads, voice cleanup, etc.
    virtual void tick() = 0;

    // Master volume scaling. 0.0 to 1.0. Applies to all subsequent
    // sound and music playback.
    virtual void setMasterVolume(float volume) = 0;

    // -- Spatial / one-shot sound effects --

    // Load and start playing a sound. The backend allocates a voice and
    // returns a handle that can be used to query / stop the sound. If
    // the load fails or the mixer is at capacity, returns an invalid
    // handle (the caller should treat that as "sound was dropped" - not
    // an error).
    [[nodiscard]] virtual SoundHandle playSoundFromFile(
        const std::string& path, const PlaySoundParams& params) = 0;

    virtual void stopSound(SoundHandle handle) = 0;
    [[nodiscard]] virtual bool isSoundPlaying(SoundHandle handle) const = 0;

    virtual void setSoundVolume(SoundHandle handle, float volume) = 0;
    virtual void setSoundPosition(SoundHandle handle, float x, float y,
                                  float z) = 0;
    virtual void setSoundPitch(SoundHandle handle, float pitch) = 0;

    // Release the voice. Idempotent on invalid handles.
    virtual void releaseSound(SoundHandle handle) = 0;

    // -- Streaming music --

    // Load and start a streaming music track. Streaming means the
    // backend reads the file incrementally rather than decoding it all
    // upfront. Use for music tracks; sound effects should use the
    // playSound* methods above.
    [[nodiscard]] virtual MusicHandle playMusicFromFile(
        const std::string& path, const PlayMusicParams& params) = 0;

    virtual void stopMusic(MusicHandle handle) = 0;
    [[nodiscard]] virtual bool isMusicPlaying(MusicHandle handle) const = 0;
    virtual void setMusicVolume(MusicHandle handle, float volume) = 0;
    virtual void setMusicPitch(MusicHandle handle, float pitch) = 0;
    virtual void releaseMusic(MusicHandle handle) = 0;

    // -- Listener (one per local splitscreen player) --

    virtual void setListenerPosition(int listenerIndex, float x, float y,
                                     float z) = 0;
    virtual void setListenerOrientation(int listenerIndex, float forwardX,
                                        float forwardY, float forwardZ,
                                        float upX, float upY, float upZ) = 0;
};

}  // namespace platform::sound
