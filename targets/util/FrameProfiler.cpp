
#ifdef ENABLE_FRAME_PROFILER

#include "FrameProfiler.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#if defined(_MSC_VER)
#define FRAME_PROFILER_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define FRAME_PROFILER_NOINLINE __attribute__((noinline))
#else
#define FRAME_PROFILER_NOINLINE
#endif

namespace {

using FrameProfilerClock = std::chrono::steady_clock;
using Bucket = FrameProfiler::Bucket;
constexpr std::uint64_t kNsPerMs = 1000ULL * 1000ULL;
constexpr std::uint64_t kReportIntervalNs = 1000ULL * 1000ULL * 1000ULL;
constexpr std::size_t kBucketCount = FrameProfiler::BucketCount();
constexpr auto kFalseTokens = std::to_array<std::string_view>({
    "0",
    "false",
    "False",
    "false",
    "no",
    "No",
    "NO",
    "off",
    "Off",
    "OFF",
});
constexpr std::array<FrameProfiler::BucketDescriptor, kBucketCount>
    kBucketDescriptors = {{
        {Bucket::Frame, "frame"},
        {Bucket::World, "world"},
        {Bucket::Terrain, "terrain"},
        {Bucket::ChunkCull, "chunkCull"},
        {Bucket::ChunkCollect, "chunkCollect"},
        {Bucket::ChunkPlayback, "chunkPlayback"},
        {Bucket::ChunkDirtyScan, "chunkDirtyScan"},
        {Bucket::ChunkRebuildSchedule, "chunkRebuildSchedule"},
        {Bucket::ChunkRebuildBody, "chunkRebuildBody"},
        {Bucket::ChunkPrepass, "chunkPrepass"},
        {Bucket::ChunkBlockShape, "chunkBlockShape"},
        {Bucket::ChunkBlockFaceCull, "chunkBlockFaceCull"},
        {Bucket::ChunkBlockLighting, "chunkBlockLighting"},
        {Bucket::ChunkBlockEmit, "chunkBlockEmit"},
        {Bucket::RenderableTileEntityCleanup, "renderableTileEntityCleanup"},
        {Bucket::TileEntityUnloadCleanup, "tileEntityUnloadCleanup"},
        {Bucket::Entity, "entities"},
        {Bucket::Particle, "particles"},
        {Bucket::WeatherSky, "weather"},
        {Bucket::UIHud, "ui"},
        {Bucket::Lightmap, "lightmap"},
    }};

struct BucketTotals {
    std::uint64_t totalNs{};
    std::uint64_t maxNs{};
    std::uint64_t calls{};

    void Record(std::uint64_t elapsedNs) noexcept {
        totalNs += elapsedNs;
        ++calls;
        if (elapsedNs > maxNs) maxNs = elapsedNs;
    }

    void Merge(const BucketTotals& other) noexcept {
        totalNs += other.totalNs;
        calls += other.calls;
        if (other.maxNs > maxNs) maxNs = other.maxNs;
    }
};

struct AtomicBucketTotals {
    std::atomic<std::uint64_t> totalNs{0};
    std::atomic<std::uint64_t> maxNs{0};
    std::atomic<std::uint64_t> calls{0};
};

struct ProfilerState {
    std::array<AtomicBucketTotals, kBucketCount> workerBuckets{};
};

struct ThreadState {
    std::uint32_t frameScopeDepth{};
    std::uint64_t windowStartNs{};
    std::array<BucketTotals, kBucketCount> localBuckets{};
};

constinit ProfilerState g_profilerState{};
constinit thread_local ThreadState t_threadState{};

static_assert(kBucketDescriptors.size() == kBucketCount);

[[nodiscard]] inline std::uint64_t nowNs() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            FrameProfilerClock::now().time_since_epoch())
            .count());
}

[[nodiscard]] constexpr double nsToMs(std::uint64_t ns) noexcept {
    return static_cast<double>(ns) / static_cast<double>(kNsPerMs);
}

[[nodiscard]] constexpr bool envSaysDisabled(std::string_view value) noexcept {
    if (value.empty()) return false;

    for (std::string_view falseToken : kFalseTokens) {
        if (value == falseToken) return true;
    }
    return false;
}

inline void updateAtomicMax(std::atomic<std::uint64_t>& value,
                            std::uint64_t candidate) noexcept {
    std::uint64_t current = value.load(std::memory_order_relaxed);
    while (current < candidate &&
           !value.compare_exchange_weak(current, candidate,
                                        std::memory_order_relaxed,
                                        std::memory_order_relaxed)) {
    }
}

inline void recordWorkerBucket(Bucket bucket,
                               std::uint64_t elapsedNs) noexcept {
    AtomicBucketTotals& state =
        g_profilerState.workerBuckets[FrameProfiler::BucketIndex(bucket)];
    state.totalNs.fetch_add(elapsedNs, std::memory_order_relaxed);
    state.calls.fetch_add(1, std::memory_order_relaxed);
    updateAtomicMax(state.maxNs, elapsedNs);
}

[[nodiscard]] inline bool isFrameThread() noexcept {
    return t_threadState.frameScopeDepth != 0;
}

FRAME_PROFILER_NOINLINE bool computeEnabled() noexcept {
    const char* const envValue = std::getenv("C4J_FRAME_PROFILER");
    if (envValue == nullptr) return true;
    return !envSaysDisabled(envValue);
}

FRAME_PROFILER_NOINLINE void emitWindowReport(
    const std::array<BucketTotals, kBucketCount>& buckets) noexcept {
    const std::uint64_t frames =
        buckets[FrameProfiler::BucketIndex(Bucket::Frame)].calls;
    if (frames == 0) return;

    std::fprintf(stderr, "[frame-prof] avg/frame(ms) frames=%llu",
                 static_cast<unsigned long long>(frames));
    for (const auto& descriptor : kBucketDescriptors) {
        const BucketTotals& bucket =
            buckets[FrameProfiler::BucketIndex(descriptor.bucket)];
        const std::string_view label = descriptor.label;
        std::fprintf(stderr, " %.*s=%.2f", static_cast<int>(label.size()),
                     label.data(), nsToMs(bucket.totalNs) / frames);
    }
    std::fputc('\n', stderr);

    std::fputs("[frame-prof] max(ms)/calls", stderr);
    for (const auto& descriptor : kBucketDescriptors) {
        const BucketTotals& bucket =
            buckets[FrameProfiler::BucketIndex(descriptor.bucket)];
        const std::string_view label = descriptor.label;
        std::fprintf(stderr, " %.*s=%.2f/%llu", static_cast<int>(label.size()),
                     label.data(), nsToMs(bucket.maxNs),
                     static_cast<unsigned long long>(bucket.calls));
    }
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

[[nodiscard]] std::array<BucketTotals, kBucketCount>
snapshotAndResetWorkerBuckets() noexcept {
    std::array<BucketTotals, kBucketCount> snapshot = {};
    for (std::size_t i = 0; i < kBucketCount; ++i) {
        AtomicBucketTotals& workerBucket = g_profilerState.workerBuckets[i];
        snapshot[i].totalNs =
            workerBucket.totalNs.exchange(0, std::memory_order_relaxed);
        snapshot[i].maxNs =
            workerBucket.maxNs.exchange(0, std::memory_order_relaxed);
        snapshot[i].calls =
            workerBucket.calls.exchange(0, std::memory_order_relaxed);
    }
    return snapshot;
}

}  // namespace

bool FrameProfiler::IsEnabled() noexcept {
    static const bool enabled = computeEnabled();
    return enabled;
}

void FrameProfiler::Record(Bucket bucket, std::uint64_t elapsedNs) noexcept {
    if (isFrameThread()) {
        t_threadState.localBuckets[BucketIndex(bucket)].Record(elapsedNs);
        return;
    }

    recordWorkerBucket(bucket, elapsedNs);
}

void FrameProfiler::EndFrame(std::uint64_t elapsedNs) noexcept {
    Record(Bucket::Frame, elapsedNs);

    ThreadState& threadState = t_threadState;
    const std::uint64_t now = nowNs();

    if (threadState.windowStartNs == 0) {
        threadState.windowStartNs = now;
        return;
    }

    if ((now - threadState.windowStartNs) < kReportIntervalNs) return;

    std::array<BucketTotals, kBucketCount> combined = threadState.localBuckets;
    const auto workerSnapshot = snapshotAndResetWorkerBuckets();

    for (std::size_t i = 0; i < kBucketCount; ++i) {
        combined[i].Merge(workerSnapshot[i]);
    }

    emitWindowReport(combined);

    threadState.windowStartNs = now;
    threadState.localBuckets = {};
}

FrameProfiler::Scope::Scope(Bucket bucket) noexcept
    : m_startNs(0), m_bucket(bucket), m_enabled(FrameProfiler::IsEnabled()) {
    if (m_enabled) m_startNs = nowNs();
}

FrameProfiler::Scope::~Scope() noexcept {
    if (!m_enabled) return;
    FrameProfiler::Record(m_bucket, nowNs() - m_startNs);
}

FrameProfiler::FrameScope::FrameScope() noexcept
    : m_startNs(0), m_enabled(false) {
    if (!FrameProfiler::IsEnabled()) return;

    m_enabled = (t_threadState.frameScopeDepth++ == 0);
    if (m_enabled) m_startNs = nowNs();
}

FrameProfiler::FrameScope::~FrameScope() noexcept {
    if (!m_enabled) {
        if (t_threadState.frameScopeDepth > 0) {
            --t_threadState.frameScopeDepth;
        }
        return;
    }

    FrameProfiler::EndFrame(nowNs() - m_startNs);

    if (t_threadState.frameScopeDepth > 0) {
        --t_threadState.frameScopeDepth;
    }
}

#endif
