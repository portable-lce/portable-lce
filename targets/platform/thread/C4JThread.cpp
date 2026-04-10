#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#if defined(_WIN32)
#include <Windows.h>
#endif

#if defined(__linux__)
#include <pthread.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "platform/thread/C4JThread.h"
#include "platform/thread/ShutdownManager.h"

class Level;

thread_local C4JThread* C4JThread::ms_currentThread = nullptr;

namespace {

constexpr int kDefaultStackSize = 65536 * 2;
constexpr int kMinimumStackSize = 16384;
constexpr auto kUnsetPriority =
    static_cast<C4JThread::ThreadPriority>(std::numeric_limits<int>::min());
constexpr int kEventQueueShutdownPollMs = 100;

const std::thread::id g_processMainThreadId = std::this_thread::get_id();

template <typename Predicate>
bool waitForCondition(std::condition_variable& condition,
                      std::unique_lock<std::mutex>& lock, int timeoutMs,
                      Predicate predicate) {
    if (timeoutMs < 0) {
        condition.wait(lock, predicate);
        return true;
    }
    return condition.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                              predicate);
}

std::uint32_t firstSetBitIndex(std::uint32_t bitMask) {
    return static_cast<std::uint32_t>(std::countr_zero(bitMask));
}

std::uint32_t buildMaskForSize(int size) {
    assert(size > 0 && size <= 32);
    if (size == 32) return 0xFFFFFFFFU;
    return (1U << static_cast<std::uint32_t>(size)) - 1U;
}

void formatThreadName(std::string& out, const char* name) {
    const char* safe = (name && name[0] != '\0') ? name : "Unnamed";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "(4J) %s", safe);
    out = buf;
}

std::int64_t getNativeThreadId() {
#if defined(__linux__)
    return static_cast<std::int64_t>(::syscall(SYS_gettid));
#else
    return 0;
#endif
}

void setThreadNamePlatform([[maybe_unused]] std::uint32_t threadId,
                           [[maybe_unused]] const char* name) {
#if defined(_WIN32)
    // Try modern API first (Windows 10 1607+).
    if (threadId == static_cast<std::uint32_t>(-1) ||
        threadId == ::GetCurrentThreadId()) {
        using SetThreadDescriptionFn = int32_t(WINAPI*)(void*, PCWSTR);
        const HMODULE kernel = ::GetModuleHandleW("Kernel32.dll");
        if (kernel) {
            const auto fn = reinterpret_cast<SetThreadDescriptionFn>(
                ::GetProcAddress(kernel, "SetThreadDescription"));
            if (fn) {
                char wide[64];
                const auto n = std::strncpy(
                    wide, name, (sizeof(wide) / sizeof(wide[0])) - 1);
                if (n != static_cast<std::size_t>(-1)) {
                    wide[n] = '\0';
                    (void)fn(::GetCurrentThread(), wide);
                    return;
                }
            }
        }
    }

    // Legacy fallback: raise exception 0x406D1388 for older MSVC debuggers.
#pragma pack(push, 8)
    struct THREADNAME_INFO {
        std::uint32_t dwType;
        const char* szName;
        std::uint32_t dwThreadID;
        std::uint32_t dwFlags;
    };
#pragma pack(pop)

    THREADNAME_INFO info{0x1000, name, threadId, 0};
    __try {
        ::RaiseException(0x406D1388, 0, sizeof(info) / sizeof(uintptr_t),
                         reinterpret_cast<uintptr_t*>(&info));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

#elif defined(__linux__)
    // pthread_setname_np limit: 16 chars including null terminator.
    char truncated[16];
    std::snprintf(truncated, sizeof(truncated), "%s", name);
    (void)::pthread_setname_np(::pthread_self(), truncated);
#endif
}

void setPriorityPlatform(std::thread& threadHandle, bool isSelf,
                         C4JThread::ThreadPriority priority,
                         std::atomic<std::int64_t>& nativeTid) {
#if defined(_WIN32)
    void* handle = nullptr;
    if (threadHandle.joinable())
        handle = threadHandle.native_handle();
    else if (isSelf)
        handle = ::GetCurrentThread();
    else
        return;
    (void)::SetThreadPriority(handle, static_cast<int>(priority));

#elif defined(__linux__)
    std::int64_t tid = 0;
    if (isSelf) {
        tid = getNativeThreadId();
        nativeTid.store(tid, std::memory_order_release);
    } else {
        tid = nativeTid.load(std::memory_order_acquire);
    }
    if (tid <= 0) return;

    using enum C4JThread::ThreadPriority;
    int niceValue = 0;
    switch (priority) {
        case TimeCritical:
            niceValue = -15;
            break;
        case Highest:
            niceValue = -10;
            break;
        case AboveNormal:
            niceValue = -5;
            break;
        case Normal:
            niceValue = 0;
            break;
        case BelowNormal:
            niceValue = 5;
            break;
        case Lowest:
            niceValue = 10;
            break;
        case Idle:
            niceValue = 19;
            break;
        default:
            niceValue = 0;
            break;
    }

    errno = 0;
    if (::setpriority(PRIO_PROCESS, static_cast<id_t>(tid), niceValue) != 0) {
        if ((errno == EACCES || errno == EPERM) && niceValue < 0) {
            (void)::setpriority(PRIO_PROCESS, static_cast<id_t>(tid), 0);
        }
    }
#else
    (void)threadHandle;
    (void)isSelf;
    (void)priority;
    (void)nativeTid;
#endif
}

}  // namespace

C4JThread::C4JThread(C4JThreadStartFunc* startFunc, void* param,
                     const char* threadName, int stackSize)
    : m_threadParam(param),
      m_startFunc(startFunc),
      m_stackSize(std::max(stackSize == 0 ? kDefaultStackSize : stackSize,
                           kMinimumStackSize)),
      m_threadName(),
      m_isRunning(false),
      m_hasStarted(false),
      m_exitCode(kStillActive),
      m_threadID(),
      m_threadHandle(),
      m_completionFlag(std::make_unique<Event>(Event::Mode::ManualClear)),
      m_requestedPriority(kUnsetPriority),
      m_nativeTid(0) {
    formatThreadName(m_threadName, threadName);
}

C4JThread::C4JThread(const char* mainThreadName)
    : m_threadParam(nullptr),
      m_startFunc(nullptr),
      m_stackSize(0),
      m_threadName(),
      m_isRunning(true),
      m_hasStarted(true),
      m_exitCode(kStillActive),
      m_threadID(std::this_thread::get_id()),
      m_threadHandle(),
      m_completionFlag(std::make_unique<Event>(Event::Mode::ManualClear)),
      m_requestedPriority(kUnsetPriority),
      m_nativeTid(getNativeThreadId()) {
    formatThreadName(m_threadName, mainThreadName);
    ms_currentThread = this;
    setCurrentThreadName(m_threadName.c_str());
}

C4JThread::~C4JThread() {
    if (m_threadHandle.joinable()) {
        if (m_threadHandle.get_id() == std::this_thread::get_id()) {
            m_threadHandle.detach();
        } else {
            m_threadHandle.join();
        }
    }

    if (ms_currentThread == this) {
        ms_currentThread = nullptr;
    }
}

C4JThread& C4JThread::getMainThreadInstance() noexcept {
    static C4JThread mainThread("Main thread");
    return mainThread;
}

void C4JThread::entryPoint(C4JThread* pThread) {
    ms_currentThread = pThread;
    pThread->m_threadID = std::this_thread::get_id();
    pThread->m_nativeTid.store(getNativeThreadId(), std::memory_order_release);

    setCurrentThreadName(pThread->m_threadName.c_str());

    const auto requestedPriority =
        pThread->m_requestedPriority.load(std::memory_order_acquire);
    if (requestedPriority != kUnsetPriority) {
        pThread->setPriority(requestedPriority);
    }

    int exitCode = 0;
    try {
        exitCode = pThread->m_startFunc
                       ? (*pThread->m_startFunc)(pThread->m_threadParam)
                       : 0;
    } catch (...) {
        exitCode = -1;
    }

    pThread->m_exitCode.store(exitCode, std::memory_order_release);
    pThread->m_isRunning.store(false, std::memory_order_release);
    pThread->m_completionFlag->set();
}

void C4JThread::run() {
    bool expected = false;
    if (!m_hasStarted.compare_exchange_strong(expected, true,
                                              std::memory_order_acq_rel)) {
        return;
    }

    m_isRunning.store(true, std::memory_order_release);
    m_exitCode.store(kStillActive, std::memory_order_release);
    m_completionFlag->clear();
    m_nativeTid.store(0, std::memory_order_release);

    m_threadHandle = std::thread(&C4JThread::entryPoint, this);
    m_threadID = m_threadHandle.get_id();

    const auto requestedPriority =
        m_requestedPriority.load(std::memory_order_acquire);
    if (requestedPriority != kUnsetPriority) {
        setPriority(requestedPriority);
    }
}

void C4JThread::setPriority(ThreadPriority priority) {
    m_requestedPriority.store(priority, std::memory_order_release);
    setPriorityPlatform(m_threadHandle, ms_currentThread == this, priority,
                        m_nativeTid);
}

std::uint32_t C4JThread::waitForCompletion(int timeoutMs) {
    const std::uint32_t result = m_completionFlag->waitForSignal(timeoutMs);
    if (result == WaitResult::Signaled && m_threadHandle.joinable()) {
        if (m_threadHandle.get_id() == std::this_thread::get_id()) {
            m_threadHandle.detach();
        } else {
            m_threadHandle.join();
        }
    }
    return result;
}

int C4JThread::getExitCode() const noexcept {
    return m_isRunning.load(std::memory_order_acquire)
               ? kStillActive
               : m_exitCode.load(std::memory_order_acquire);
}

C4JThread* C4JThread::getCurrentThread() noexcept {
    if (ms_currentThread) return ms_currentThread;
    if (std::this_thread::get_id() == g_processMainThreadId)
        return &getMainThreadInstance();
    return nullptr;
}

bool C4JThread::isMainThread() noexcept {
    return std::this_thread::get_id() == g_processMainThreadId;
}

void C4JThread::setThreadName(std::uint32_t threadId, const char* threadName) {
    const char* safe =
        (threadName && threadName[0] != '\0') ? threadName : "(4J) Unnamed";
    setThreadNamePlatform(threadId, safe);
}

void C4JThread::setCurrentThreadName(const char* threadName) {
    setThreadName(static_cast<std::uint32_t>(-1), threadName);
}

C4JThread::Event::Event(Mode mode)
    : m_mode(mode), m_mutex(), m_condition(), m_signaled(false) {}

void C4JThread::Event::set() {
    {
        std::lock_guard lock(m_mutex);
        m_signaled = true;
    }
    if (m_mode == Mode::AutoClear)
        m_condition.notify_one();
    else
        m_condition.notify_all();
}

void C4JThread::Event::clear() {
    std::lock_guard lock(m_mutex);
    m_signaled = false;
}

std::uint32_t C4JThread::Event::waitForSignal(int timeoutMs) {
    std::unique_lock lock(m_mutex);
    if (!waitForCondition(m_condition, lock, timeoutMs,
                          [this] { return m_signaled; })) {
        return WaitResult::Timeout;
    }
    if (m_mode == Mode::AutoClear) m_signaled = false;
    return WaitResult::Signaled;
}

C4JThread::EventArray::EventArray(int size, Mode mode)
    : m_size(size), m_mode(mode), m_signaledMask(0U) {
    assert(m_size > 0 && m_size <= 32);
}

void C4JThread::EventArray::set(int index) {
    assert(index >= 0 && index < m_size);
    const std::uint32_t bit = 1U << static_cast<std::uint32_t>(index);
    m_signaledMask.fetch_or(bit, std::memory_order_release);
    m_signaledMask.notify_all();
}

void C4JThread::EventArray::clear(int index) {
    assert(index >= 0 && index < m_size);
    const std::uint32_t bit = 1U << static_cast<std::uint32_t>(index);
    m_signaledMask.fetch_and(~bit, std::memory_order_relaxed);
}

void C4JThread::EventArray::setAll() {
    const std::uint32_t mask = buildMaskForSize(m_size);
    m_signaledMask.fetch_or(mask, std::memory_order_release);
    m_signaledMask.notify_all();
}

void C4JThread::EventArray::clearAll() {
    m_signaledMask.store(0U, std::memory_order_relaxed);
}

namespace {

// Polling fallback for finite-timeout waits; std::atomic::wait has no timed
// overload until C++26.
template <class Predicate>
bool waitForMaskTimed(std::atomic<std::uint32_t>& mask, int timeoutMs,
                      Predicate&& predicate) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
    auto interval = std::chrono::microseconds(100);
    constexpr auto maxInterval = std::chrono::milliseconds(2);
    while (clock::now() < deadline) {
        if (predicate(mask.load(std::memory_order_acquire))) return true;
        std::this_thread::sleep_for(interval);
        if (interval < maxInterval) interval *= 2;
    }
    return predicate(mask.load(std::memory_order_acquire));
}

}  // namespace

std::uint32_t C4JThread::EventArray::waitForSingle(int index, int timeoutMs) {
    assert(index >= 0 && index < m_size);
    const std::uint32_t bitMask = 1U << static_cast<std::uint32_t>(index);
    const auto predicate = [bitMask](std::uint32_t cur) {
        return (cur & bitMask) != 0U;
    };

    if (timeoutMs == kInfiniteTimeout) {
        std::uint32_t cur = m_signaledMask.load(std::memory_order_acquire);
        while (!predicate(cur)) {
            m_signaledMask.wait(cur, std::memory_order_relaxed);
            cur = m_signaledMask.load(std::memory_order_acquire);
        }
    } else if (!waitForMaskTimed(m_signaledMask, timeoutMs, predicate)) {
        return WaitResult::Timeout;
    }

    if (m_mode == Mode::AutoClear) {
        m_signaledMask.fetch_and(~bitMask, std::memory_order_release);
    }
    return WaitResult::Signaled;
}

std::uint32_t C4JThread::EventArray::waitForAll(int timeoutMs) {
    const std::uint32_t bitMask = buildMaskForSize(m_size);
    const auto predicate = [bitMask](std::uint32_t cur) {
        return (cur & bitMask) == bitMask;
    };

    if (timeoutMs == kInfiniteTimeout) {
        std::uint32_t cur = m_signaledMask.load(std::memory_order_acquire);
        while (!predicate(cur)) {
            m_signaledMask.wait(cur, std::memory_order_relaxed);
            cur = m_signaledMask.load(std::memory_order_acquire);
        }
    } else if (!waitForMaskTimed(m_signaledMask, timeoutMs, predicate)) {
        return WaitResult::Timeout;
    }

    if (m_mode == Mode::AutoClear) {
        m_signaledMask.fetch_and(~bitMask, std::memory_order_release);
    }
    return WaitResult::Signaled;
}

std::uint32_t C4JThread::EventArray::waitForAny(int timeoutMs) {
    const std::uint32_t bitMask = buildMaskForSize(m_size);
    const auto predicate = [bitMask](std::uint32_t cur) {
        return (cur & bitMask) != 0U;
    };

    if (timeoutMs == kInfiniteTimeout) {
        std::uint32_t cur = m_signaledMask.load(std::memory_order_acquire);
        while (!predicate(cur)) {
            m_signaledMask.wait(cur, std::memory_order_relaxed);
            cur = m_signaledMask.load(std::memory_order_acquire);
        }
    } else if (!waitForMaskTimed(m_signaledMask, timeoutMs, predicate)) {
        return WaitResult::Timeout;
    }

    const std::uint32_t cur = m_signaledMask.load(std::memory_order_acquire);
    const std::uint32_t readyIndex = firstSetBitIndex(cur & bitMask);
    if (m_mode == Mode::AutoClear) {
        m_signaledMask.fetch_and(~(1U << readyIndex),
                                 std::memory_order_release);
    }
    return WaitResult::Signaled + readyIndex;
}

C4JThread::EventQueue::EventQueue(UpdateFunc* updateFunc,
                                  ThreadInitFunc* threadInitFunc,
                                  const char* threadName)
    : m_thread(),
      m_queue(),
      m_mutex(),
      m_queueCondition(),
      m_drainedCondition(),
      m_updateFunc(updateFunc),
      m_threadInitFunc(threadInitFunc),
      m_threadName(threadName ? threadName : "Unnamed"),
      m_priority(kUnsetPriority),
      m_busy(false),
      m_initOnce(),
      m_stopRequested(false) {
    assert(m_updateFunc);
}

C4JThread::EventQueue::~EventQueue() {
    m_stopRequested.store(true, std::memory_order_release);
    m_queueCondition.notify_all();
    if (m_thread) (void)m_thread->waitForCompletion(kInfiniteTimeout);
}

void C4JThread::EventQueue::setPriority(ThreadPriority priority) {
    m_priority = priority;
    if (m_thread) m_thread->setPriority(priority);
}

void C4JThread::EventQueue::init() {
    std::call_once(m_initOnce, [this]() {
        m_thread =
            std::make_unique<C4JThread>(threadFunc, this, m_threadName.c_str());
        if (m_priority != kUnsetPriority) m_thread->setPriority(m_priority);
        m_thread->run();
    });
}

void C4JThread::EventQueue::sendEvent(Level* pLevel) {
    init();
    if (m_stopRequested.load(std::memory_order_acquire)) return;
    {
        std::lock_guard lock(m_mutex);
        m_queue.push(pLevel);
    }
    m_queueCondition.notify_one();
}

void C4JThread::EventQueue::waitForFinish() {
    init();
    std::unique_lock lock(m_mutex);
    m_drainedCondition.wait(lock,
                            [this] { return m_queue.empty() && !m_busy; });
}

int C4JThread::EventQueue::threadFunc(void* lpParam) {
    static_cast<EventQueue*>(lpParam)->threadPoll();
    return 0;
}

void C4JThread::EventQueue::threadPoll() {
    ShutdownManager::HasStarted(ShutdownManager::eEventQueueThreads);

    if (m_threadInitFunc) {
        try {
            m_threadInitFunc();
        } catch (...) {
        }
    }

    while (!m_stopRequested.load(std::memory_order_acquire) &&
           ShutdownManager::ShouldRun(ShutdownManager::eEventQueueThreads)) {
        void* updateParam = nullptr;

        {
            std::unique_lock lock(m_mutex);
            m_queueCondition.wait_for(
                lock, std::chrono::milliseconds(kEventQueueShutdownPollMs),
                [this] {
                    return m_stopRequested.load(std::memory_order_acquire) ||
                           !m_queue.empty();
                });

            if (m_stopRequested.load(std::memory_order_acquire)) break;
            if (m_queue.empty()) continue;

            m_busy = true;
            updateParam = m_queue.front();
            m_queue.pop();
        }

        try {
            m_updateFunc(updateParam);
        } catch (...) {
        }

        {
            std::lock_guard lock(m_mutex);
            m_busy = false;
            if (m_queue.empty()) m_drainedCondition.notify_all();
        }
    }

    {
        std::lock_guard lock(m_mutex);
        m_busy = false;
        std::queue<void*> empty;
        m_queue.swap(empty);
    }
    m_drainedCondition.notify_all();

    ShutdownManager::HasFinished(ShutdownManager::eEventQueueThreads);
}
