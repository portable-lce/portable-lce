#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

#include "platform/thread/C4JThread.h"

class ShutdownManager {
public:
    enum EThreadId {
        eMainThread,
        eLeaderboardThread,
        eCommerceThread,
        ePostProcessThread,
        eRunUpdateThread,
        eRenderChunkUpdateThread,
        eServerThread,
        ePlatformStorageThreads,
        eConnectionReadThreads,
        eConnectionWriteThreads,
        eEventQueueThreads,

        eThreadIdCount
    };

    static void Initialise();
    static void StartShutdown();
    static void MainThreadHandleShutdown();

    static void HasStarted(EThreadId threadId);
    static void HasStarted(EThreadId threadId,
                           C4JThread::EventArray* eventArray);
    static bool ShouldRun(EThreadId threadId);
    static void HasFinished(EThreadId threadId);

private:
    struct GroupState {
        std::size_t started = 0;
        std::size_t running = 0;
    };

    struct State {
        std::mutex mutex;
        std::condition_variable condition;
        std::array<GroupState, eThreadIdCount> groups{};
        std::atomic<bool> shutdownRequested{false};
    };

    static State& GetState();
};