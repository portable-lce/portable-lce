#include "StubPlatformNetwork.h"

#include "platform/network/network.h"

namespace platform_internal {
IPlatformNetwork& PlatformNetwork_get() {
    static StubPlatformNetwork instance;
    return instance;
}
}  // namespace platform_internal
