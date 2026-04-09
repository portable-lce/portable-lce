#include "platform/XboxStubs.h"

#include "platform/PlatformTypes.h"

bool IsEqualXUID(PlayerUID a, PlayerUID b) { return a == b; }

uint32_t XUserGetSigninInfo(uint32_t dwUserIndex, uint32_t dwFlags,
                            PXUSER_SIGNIN_INFO pSigninInfo) {
    return 0;
}

const char* CXuiStringTable::Lookup(const char* szId) { return szId; }
const char* CXuiStringTable::Lookup(uint32_t nIndex) { return "String"; }
void CXuiStringTable::Clear() {}
int32_t CXuiStringTable::Load(const char* szId) { return 0; }

uint32_t XGetLanguage() { return 1; }
uint32_t XGetLocale() { return 0; }
uint32_t XEnableGuestSignin(bool fEnable) { return 0; }
