#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using C4JThreadStartFunc = int(void* lpThreadParameter);

class Level;

class C4JThread {
public:
    struct WaitResult {
        static constexpr std::uint32_t Signaled = 0;
        static constexpr std::uint32_t Timeout = 258;
    };

    enum class ThreadPriority : int {
        Idle = -15,
        Lowest = -2,
        BelowNormal = -1,
        Normal = 0,
        AboveNormal = 1,
        Highest = 2,
        TimeCritical = 15
    };

    static constexpr int kInfiniteTimeout = -1;
    static constexpr int kStillActive = 259;

    class Event {
    public:
        enum class Mode { AutoClear, ManualClear };

        explicit Event(Mode mode = Mode::AutoClear);
        ~Event() = default;

        void set();
        void clear();
        std::uint32_t waitForSignal(int timeoutMs);

    private:
        Mode m_mode;
        std::mutex m_mutex;
        std::condition_variable m_condition;
        bool m_signaled;
    };

    // Lock-free bitmask of up to 32 events; waiters block via std::atomic::wait.
    class EventArray {
    public:
        enum class Mode { AutoClear, ManualClear };

        explicit EventArray(int size, Mode mode = Mode::AutoClear);

        void set(int index);
        void clear(int index);
        void setAll();
        void clearAll();
        std::uint32_t waitForAll(int timeoutMs);
        std::uint32_t waitForAny(int timeoutMs);
        std::uint32_t waitForSingle(int index, int timeoutMs);

    private:
        int m_size;
        Mode m_mode;
        std::atomic<std::uint32_t> m_signaledMask;
    };

    class EventQueue {
    public:
        using UpdateFunc = void(void* lpParameter);
        using ThreadInitFunc = void();

        EventQueue(UpdateFunc* updateFunc, ThreadInitFunc* threadInitFunc,
                   const char* threadName);
        ~EventQueue();

        EventQueue(const EventQueue&) = delete;
        EventQueue& operator=(const EventQueue&) = delete;

        void setPriority(ThreadPriority priority);
        void sendEvent(Level* pLevel);
        void waitForFinish();

    private:
        void init();
        static int threadFunc(void* lpParam);
        void threadPoll();

        std::unique_ptr<C4JThread> m_thread;
        std::queue<void*> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_queueCondition;
        std::condition_variable m_drainedCondition;
        UpdateFunc* m_updateFunc;
        ThreadInitFunc* m_threadInitFunc;
        std::string m_threadName;
        ThreadPriority m_priority;
        bool m_busy;
        std::once_flag m_initOnce;
        std::atomic<bool> m_stopRequested;
    };

    C4JThread(C4JThreadStartFunc* startFunc, void* param,
              const char* threadName, int stackSize = 0);
    explicit C4JThread(const char* mainThreadName);
    ~C4JThread();

    C4JThread(const C4JThread&) = delete;
    C4JThread& operator=(const C4JThread&) = delete;

    void run();

    [[nodiscard]] bool isRunning() const noexcept {
        return m_isRunning.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool hasStarted() const noexcept {
        return m_hasStarted.load(std::memory_order_acquire);
    }

    void setPriority(ThreadPriority priority);

    std::uint32_t waitForCompletion(int timeoutMs);
    [[nodiscard]] int getExitCode() const noexcept;

    [[nodiscard]] const char* getName() const noexcept {
        return m_threadName.c_str();
    }

    static C4JThread* getCurrentThread() noexcept;
    static bool isMainThread() noexcept;

    static const char* getCurrentThreadName() noexcept {
        const C4JThread* pThread = getCurrentThread();
        return pThread ? pThread->getName() : "(4J) Unknown thread";
    }

    static void setThreadName(std::uint32_t threadId, const char* threadName);
    static void setCurrentThreadName(const char* threadName);

    // TODO(C++26): When we switch to C++26, replace EventQueue with
    // std::execution (senders/receivers) for structured concurrency.
    // TODO(C++26): When we switch to C++26, use std::hazard_pointer / std::rcu
    // for lock-free data structure reclamation.

private:
    static void entryPoint(C4JThread* pThread);
    static C4JThread& getMainThreadInstance() noexcept;

    void* m_threadParam;
    C4JThreadStartFunc* m_startFunc;
    int m_stackSize;
    std::string m_threadName;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_hasStarted;
    std::atomic<int> m_exitCode;

    std::thread::id m_threadID;
    std::thread m_threadHandle;
    std::unique_ptr<Event> m_completionFlag;

    std::atomic<ThreadPriority> m_requestedPriority;
    std::atomic<std::int64_t> m_nativeTid;

    static thread_local C4JThread* ms_currentThread;
};
