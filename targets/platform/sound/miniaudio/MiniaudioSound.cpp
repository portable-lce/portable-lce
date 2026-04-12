#include "MiniaudioSound.h"

#include <atomic>
#include <unordered_map>

#if defined(__APPLE__)
#include <miniaudio/miniaudio.h>
#else
#include "miniaudio.h"
#endif
#include "platform/sound/sound.h"

namespace platform::sound::miniaudio {

namespace {

// Each loaded sound voice. Owned by the State's map; the SoundHandle
// the caller holds is the integer key.
struct Voice {
    ma_sound sound{};
    bool inUse = false;
};

}  // namespace

struct State {
    ma_engine engine{};
    ma_engine_config engineConfig{};
    bool engineReady = false;

    // Voice pool. Sound handles are keys; the backend allocates a fresh
    // id for every play call. Caller releases via releaseSound() (or
    // we'll auto-release when the sound finishes - tick() handles that).
    std::unordered_map<std::uint32_t, std::unique_ptr<Voice>> sounds;
    std::unordered_map<std::uint32_t, std::unique_ptr<Voice>> music;
    std::atomic<std::uint32_t> nextHandleId{1};
};

MiniaudioSound::MiniaudioSound() : m_state(std::make_unique<State>()) {}
MiniaudioSound::~MiniaudioSound() {
    if (m_state && m_state->engineReady) {
        shutdown();
    }
}
MiniaudioSound::MiniaudioSound(MiniaudioSound&&) noexcept = default;
MiniaudioSound& MiniaudioSound::operator=(MiniaudioSound&&) noexcept = default;

void MiniaudioSound::init(int listenerCount) {
    if (m_state->engineReady) return;
    m_state->engineConfig = ma_engine_config_init();
    m_state->engineConfig.listenerCount =
        listenerCount > 0 ? static_cast<ma_uint32>(listenerCount) : 1;
    if (ma_engine_init(&m_state->engineConfig, &m_state->engine) !=
        MA_SUCCESS) {
        return;
    }
    ma_engine_set_volume(&m_state->engine, 1.0f);
    m_state->engineReady = true;
}

void MiniaudioSound::shutdown() {
    if (!m_state->engineReady) return;
    // Tear down all live voices first.
    for (auto& [id, voice] : m_state->sounds) {
        if (voice && voice->inUse) {
            ma_sound_uninit(&voice->sound);
        }
    }
    m_state->sounds.clear();
    for (auto& [id, voice] : m_state->music) {
        if (voice && voice->inUse) {
            ma_sound_uninit(&voice->sound);
        }
    }
    m_state->music.clear();
    ma_engine_uninit(&m_state->engine);
    m_state->engineReady = false;
}

void MiniaudioSound::tick() {
    if (!m_state->engineReady) return;
    // Reap finished one-shot sounds (non-looping, not currently playing).
    // We don't auto-reap music here; music is explicitly stopped by the
    // game's music system.
    for (auto it = m_state->sounds.begin(); it != m_state->sounds.end();) {
        auto& voice = it->second;
        if (voice && voice->inUse && !ma_sound_is_playing(&voice->sound) &&
            !ma_sound_is_looping(&voice->sound)) {
            ma_sound_uninit(&voice->sound);
            voice->inUse = false;
            it = m_state->sounds.erase(it);
        } else {
            ++it;
        }
    }
}

void MiniaudioSound::setMasterVolume(float volume) {
    if (!m_state->engineReady) return;
    ma_engine_set_volume(&m_state->engine, volume);
}

SoundHandle MiniaudioSound::playSoundFromFile(const std::string& path,
                                              const PlaySoundParams& params) {
    if (!m_state->engineReady) return {};
    auto voice = std::make_unique<Voice>();
    if (ma_sound_init_from_file(&m_state->engine, path.c_str(),
                                MA_SOUND_FLAG_ASYNC, nullptr, nullptr,
                                &voice->sound) != MA_SUCCESS) {
        return {};
    }
    ma_sound_set_spatialization_enabled(&voice->sound,
                                        params.spatial ? MA_TRUE : MA_FALSE);
    ma_sound_set_min_distance(&voice->sound, params.minDistance);
    ma_sound_set_max_distance(&voice->sound, params.maxDistance);
    ma_sound_set_volume(&voice->sound, params.volume);
    ma_sound_set_pitch(&voice->sound, params.pitch);
    ma_sound_set_position(&voice->sound, params.x, params.y, params.z);
    ma_sound_set_looping(&voice->sound, params.looping ? MA_TRUE : MA_FALSE);
    ma_sound_start(&voice->sound);
    voice->inUse = true;

    SoundHandle handle{m_state->nextHandleId.fetch_add(1)};
    m_state->sounds.emplace(handle.id, std::move(voice));
    return handle;
}

void MiniaudioSound::stopSound(SoundHandle handle) {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end() || !it->second) return;
    if (it->second->inUse) {
        ma_sound_stop(&it->second->sound);
    }
}

bool MiniaudioSound::isSoundPlaying(SoundHandle handle) const {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end() || !it->second || !it->second->inUse) {
        return false;
    }
    return ma_sound_is_playing(&it->second->sound) != MA_FALSE;
}

void MiniaudioSound::setSoundVolume(SoundHandle handle, float volume) {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end() || !it->second || !it->second->inUse)
        return;
    ma_sound_set_volume(&it->second->sound, volume);
}

void MiniaudioSound::setSoundPosition(SoundHandle handle, float x, float y,
                                      float z) {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end() || !it->second || !it->second->inUse)
        return;
    ma_sound_set_position(&it->second->sound, x, y, z);
}

void MiniaudioSound::setSoundPitch(SoundHandle handle, float pitch) {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end() || !it->second || !it->second->inUse)
        return;
    ma_sound_set_pitch(&it->second->sound, pitch);
}

void MiniaudioSound::releaseSound(SoundHandle handle) {
    auto it = m_state->sounds.find(handle.id);
    if (it == m_state->sounds.end()) return;
    if (it->second && it->second->inUse) {
        ma_sound_uninit(&it->second->sound);
        it->second->inUse = false;
    }
    m_state->sounds.erase(it);
}

MusicHandle MiniaudioSound::playMusicFromFile(const std::string& path,
                                              const PlayMusicParams& params) {
    if (!m_state->engineReady) return {};
    auto voice = std::make_unique<Voice>();
    if (ma_sound_init_from_file(&m_state->engine, path.c_str(),
                                MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                &voice->sound) != MA_SUCCESS) {
        return {};
    }
    ma_sound_set_spatialization_enabled(&voice->sound, MA_FALSE);
    ma_sound_set_volume(&voice->sound, params.volume);
    ma_sound_set_pitch(&voice->sound, params.pitch);
    ma_sound_set_looping(&voice->sound, params.looping ? MA_TRUE : MA_FALSE);
    ma_sound_start(&voice->sound);
    voice->inUse = true;

    MusicHandle handle{m_state->nextHandleId.fetch_add(1)};
    m_state->music.emplace(handle.id, std::move(voice));
    return handle;
}

void MiniaudioSound::stopMusic(MusicHandle handle) {
    auto it = m_state->music.find(handle.id);
    if (it == m_state->music.end() || !it->second) return;
    if (it->second->inUse) {
        ma_sound_stop(&it->second->sound);
    }
}

bool MiniaudioSound::isMusicPlaying(MusicHandle handle) const {
    auto it = m_state->music.find(handle.id);
    if (it == m_state->music.end() || !it->second || !it->second->inUse)
        return false;
    return ma_sound_is_playing(&it->second->sound) != MA_FALSE;
}

void MiniaudioSound::setMusicVolume(MusicHandle handle, float volume) {
    auto it = m_state->music.find(handle.id);
    if (it == m_state->music.end() || !it->second || !it->second->inUse) return;
    ma_sound_set_volume(&it->second->sound, volume);
}

void MiniaudioSound::setMusicPitch(MusicHandle handle, float pitch) {
    auto it = m_state->music.find(handle.id);
    if (it == m_state->music.end() || !it->second || !it->second->inUse) return;
    ma_sound_set_pitch(&it->second->sound, pitch);
}

void MiniaudioSound::releaseMusic(MusicHandle handle) {
    auto it = m_state->music.find(handle.id);
    if (it == m_state->music.end()) return;
    if (it->second && it->second->inUse) {
        ma_sound_uninit(&it->second->sound);
        it->second->inUse = false;
    }
    m_state->music.erase(it);
}

void MiniaudioSound::setListenerPosition(int listenerIndex, float x, float y,
                                         float z) {
    if (!m_state->engineReady) return;
    ma_engine_listener_set_position(
        &m_state->engine, static_cast<ma_uint32>(listenerIndex), x, y, z);
}

void MiniaudioSound::setListenerOrientation(int listenerIndex, float forwardX,
                                            float forwardY, float forwardZ,
                                            float upX, float upY, float upZ) {
    if (!m_state->engineReady) return;
    ma_engine_listener_set_direction(&m_state->engine,
                                     static_cast<ma_uint32>(listenerIndex),
                                     forwardX, forwardY, forwardZ);
    ma_engine_listener_set_world_up(
        &m_state->engine, static_cast<ma_uint32>(listenerIndex), upX, upY, upZ);
}

}  // namespace platform::sound::miniaudio

namespace platform_internal {
::platform::sound::IPlatformSound& PlatformSound_get() {
    static ::platform::sound::miniaudio::MiniaudioSound instance;
    return instance;
}
}  // namespace platform_internal
