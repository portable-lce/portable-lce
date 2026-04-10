#pragma once

#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

#include "platform/PlatformTypes.h"

inline constexpr int MINECRAFT_NET_MAX_PLAYERS = 8;

static_assert(
    MINECRAFT_NET_MAX_PLAYERS <= std::numeric_limits<std::uint8_t>::max(),
    "MINECRAFT_NET_MAX_PLAYERS must fit in the 8-bit network protocol");

using SessionID = uint64_t;
using GameSessionUID = PlayerUID;
class INVITE_INFO;

struct XRNM_SEND_BUFFER {
    uint32_t dwDataSize;
    uint8_t* pbyData;
};

template <typename T>
class XLockFreeStack {
    std::vector<T*> intStack;
    std::mutex m_cs;

public:
    XLockFreeStack() = default;
    ~XLockFreeStack() = default;
    void Initialize() {}
    void Push(T* data) {
        std::lock_guard<std::mutex> lock(m_cs);
        intStack.push_back(data);
    }
    T* Pop() {
        std::lock_guard<std::mutex> lock(m_cs);
        if (intStack.size()) {
            T* ret = intStack.back();
            intStack.pop_back();
            return ret;
        }
        return nullptr;
    }
};

struct XNQOSINFO {
    uint8_t bFlags;
    uint8_t bReserved;
    uint16_t cProbesXmit;
    uint16_t cProbesRecv;
    uint16_t cbData;
    uint8_t* pbData;
    uint16_t wRttMinInMsecs;
    uint16_t wRttMedInMsecs;
    uint32_t dwUpBitsPerSec;
    uint32_t dwDnBitsPerSec;
};

struct XNQOS {
    uint32_t cxnqos;
    uint32_t cxnqosPending;
    XNQOSINFO axnqosinfo[1];
};

struct XOVERLAPPED {};

struct XSESSION_SEARCHRESULT {};

struct XUSER_CONTEXT {
    uint32_t dwContextId;
    uint32_t dwValue;
};

struct XSESSION_SEARCHRESULT_HEADER {
    uint32_t dwSearchResults;
    XSESSION_SEARCHRESULT* pResults;
};
