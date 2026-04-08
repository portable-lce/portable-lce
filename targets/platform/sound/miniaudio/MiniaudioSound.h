#pragma once

#include <memory>

#include "platform/sound/IPlatformSound.h"

// Forward-declare the miniaudio state struct so this header doesn't
// need to include miniaudio.h - keeps the include footprint small for
// any consumer of the backend header.
namespace platform::sound::miniaudio {

struct State;

class MiniaudioSound : public IPlatformSound {
public:
    MiniaudioSound();
    ~MiniaudioSound() override;

    // Move-only - the engine owns hardware state.
    MiniaudioSound(MiniaudioSound&&) noexcept;
    MiniaudioSound& operator=(MiniaudioSound&&) noexcept;
    MiniaudioSound(const MiniaudioSound&) = delete;
    MiniaudioSound& operator=(const MiniaudioSound&) = delete;

    // -- IPlatformSound --

    void init(int listenerCount) override;
    void shutdown() override;
    void tick() override;

    void setMasterVolume(float volume) override;

    [[nodiscard]] SoundHandle playSoundFromFile(
        const std::string& path, const PlaySoundParams& params) override;
    void stopSound(SoundHandle handle) override;
    [[nodiscard]] bool isSoundPlaying(SoundHandle handle) const override;
    void setSoundVolume(SoundHandle handle, float volume) override;
    void setSoundPosition(SoundHandle handle, float x, float y,
                          float z) override;
    void setSoundPitch(SoundHandle handle, float pitch) override;
    void releaseSound(SoundHandle handle) override;

    [[nodiscard]] MusicHandle playMusicFromFile(
        const std::string& path, const PlayMusicParams& params) override;
    void stopMusic(MusicHandle handle) override;
    [[nodiscard]] bool isMusicPlaying(MusicHandle handle) const override;
    void setMusicVolume(MusicHandle handle, float volume) override;
    void setMusicPitch(MusicHandle handle, float pitch) override;
    void releaseMusic(MusicHandle handle) override;

    void setListenerPosition(int listenerIndex, float x, float y,
                             float z) override;
    void setListenerOrientation(int listenerIndex, float forwardX,
                                float forwardY, float forwardZ, float upX,
                                float upY, float upZ) override;

private:
    std::unique_ptr<State> m_state;
};

}  // namespace platform::sound::miniaudio
