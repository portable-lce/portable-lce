#pragma once

#ifdef ENABLE_FRAME_PROFILER

#include <cstddef>
#include <cstdint>
#include <utility>

class FrameProfiler {
public:
    enum class Bucket : std::uint8_t {
        Frame,
        World,
        Terrain,
        ChunkCull,
        ChunkCollect,
        ChunkPlayback,
        ChunkDirtyScan,
        ChunkRebuildSchedule,
        ChunkRebuildBody,
        ChunkPrepass,
        ChunkBlockShape,
        ChunkBlockFaceCull,
        ChunkBlockLighting,
        ChunkBlockEmit,
        RenderableTileEntityCleanup,
        TileEntityUnloadCleanup,
        Entity,
        Particle,
        WeatherSky,
        UIHud,
        Lightmap,
        Count,
    };

    struct BucketDescriptor {
        Bucket bucket;
        const char* label;
    };

    [[nodiscard]] static constexpr std::size_t BucketIndex(
        Bucket bucket) noexcept {
        return static_cast<size_t>(bucket);
    }

    [[nodiscard]] static constexpr std::size_t BucketCount() noexcept {
        return BucketIndex(Bucket::Count);
    }

    [[nodiscard]] static bool IsEnabled() noexcept;

    class Scope {
    public:
        explicit Scope(Bucket bucket) noexcept;
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;
        ~Scope() noexcept;

    private:
        std::uint64_t m_startNs;
        Bucket m_bucket;
        bool m_enabled;
    };

    class FrameScope {
    public:
        FrameScope() noexcept;
        FrameScope(const FrameScope&) = delete;
        FrameScope& operator=(const FrameScope&) = delete;
        FrameScope(FrameScope&&) = delete;
        FrameScope& operator=(FrameScope&&) = delete;
        ~FrameScope() noexcept;

    private:
        std::uint64_t m_startNs;
        bool m_enabled;
    };

private:
    static void Record(Bucket bucket, std::uint64_t elapsedNs) noexcept;
    static void EndFrame(std::uint64_t elapsedNs) noexcept;
};

#define FRAME_PROFILE_CONCAT_INNER(a, b) a##b
#define FRAME_PROFILE_CONCAT(a, b) FRAME_PROFILE_CONCAT_INNER(a, b)
#define FRAME_PROFILE_SCOPE(bucket_name)       \
    FrameProfiler::Scope FRAME_PROFILE_CONCAT( \
        frameProfileScope_, __LINE__)(FrameProfiler::Bucket::bucket_name)
#define FRAME_PROFILE_FRAME_SCOPE()                                         \
    FrameProfiler::FrameScope FRAME_PROFILE_CONCAT(frameProfileFrameScope_, \
                                                   __LINE__)

#else

#define FRAME_PROFILE_SCOPE(bucket_name) ((void)0)
#define FRAME_PROFILE_FRAME_SCOPE() ((void)0)

#endif
