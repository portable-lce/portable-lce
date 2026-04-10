// Linux stub implementations for ShutdownManager
// The PS3/PSVita versions have full implementations; on Linux these are no-ops.
#include "platform/thread/ShutdownManager.h"

#include "platform/thread/C4JThread.h"

void ShutdownManager::Initialise() {}
void ShutdownManager::StartShutdown() {}
void ShutdownManager::MainThreadHandleShutdown() {}

void ShutdownManager::HasStarted(ShutdownManager::EThreadId /*threadId*/) {}
void ShutdownManager::HasStarted(ShutdownManager::EThreadId /*threadId*/,
                                 C4JThread::EventArray* /*eventArray*/) {}
bool ShutdownManager::ShouldRun(ShutdownManager::EThreadId /*threadId*/) {
    return true;
}
void ShutdownManager::HasFinished(ShutdownManager::EThreadId /*threadId*/) {}
