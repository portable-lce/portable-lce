#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

// Shared value types used by platform interfaces. These are NOT interfaces
// themselves — they are data carriers that cross the platform boundary.

struct ImageFileBuffer {
    enum EImageType { e_typePNG, e_typeJPG };

    ImageFileBuffer() = default;
    ImageFileBuffer(EImageType type, std::size_t size)
        : m_type(type),
          m_pBuffer(size > 0 ? std::make_unique<std::byte[]>(size) : nullptr),
          m_bufferSize(static_cast<int>(size)) {}

    // move-only
    ImageFileBuffer(ImageFileBuffer&&) noexcept = default;
    ImageFileBuffer& operator=(ImageFileBuffer&&) noexcept = default;
    ImageFileBuffer(const ImageFileBuffer&) = delete;
    ImageFileBuffer& operator=(const ImageFileBuffer&) = delete;

    [[nodiscard]] EImageType GetType() const { return m_type; }
    [[nodiscard]] std::byte* GetBufferPointer() const { return m_pBuffer.get(); }
    [[nodiscard]] int GetBufferSize() const { return m_bufferSize; }
    [[nodiscard]] bool Allocated() const { return m_pBuffer != nullptr; }
    void Release() {
        m_pBuffer.reset();
        m_bufferSize = 0;
    }

    EImageType m_type{e_typePNG};
    std::unique_ptr<std::byte[]> m_pBuffer;
    int m_bufferSize = 0;
};

struct D3DXIMAGE_INFO {
    int Width;
    int Height;
};

struct XSOCIAL_PREVIEWIMAGE {
    std::uint8_t* pBytes;
    std::uint32_t Pitch;
    std::uint32_t Width;
    std::uint32_t Height;
};
using PXSOCIAL_PREVIEWIMAGE = XSOCIAL_PREVIEWIMAGE*;

struct STRING_VERIFY_RESPONSE {
    std::uint16_t wNumStrings;
    int* pStringResult;
};

enum class EKeyboardResult {
    Pending,
    Cancelled,
    ResultAccept,
    ResultDecline,
};

// Profile-related enums at file scope.
enum class EAwardType {
    Achievement = 0,
    GamerPic,
    Theme,
    AvatarItem,
};

inline constexpr int XUSER_INDEX_ANY = 255;
inline constexpr int XUSER_MAX_COUNT = 4;
inline constexpr int XUSER_NAME_SIZE = 32;
inline constexpr int XUSER_INDEX_FOCUS = 254;

// Maximum local (split-screen) players. Same as XUSER_MAX_COUNT; kept as a
// separate name because gameplay code talks about "local players" rather
// than Xbox user slots.
#define MAX_LOCAL_PLAYERS 4

using PlayerUID = unsigned long long;
using PPlayerUID = PlayerUID*;
inline constexpr PlayerUID INVALID_XUID = 0;

class CXuiStringTable;

inline constexpr int XCONTENT_MAX_DISPLAYNAME_LENGTH = 256;
inline constexpr int XCONTENT_MAX_FILENAME_LENGTH = 256;

using XCONTENTDEVICEID = int;

struct XCONTENT_DATA {
    XCONTENTDEVICEID DeviceID;
    std::uint32_t dwContentType;
    char szDisplayName[XCONTENT_MAX_DISPLAYNAME_LENGTH];
    char szFileName[XCONTENT_MAX_FILENAME_LENGTH];
};
using PXCONTENT_DATA = XCONTENT_DATA*;

inline constexpr int XMARKETPLACE_CONTENT_ID_LEN = 4;

struct XMARKETPLACE_CONTENTOFFER_INFO {
    std::uint64_t qwOfferID;
    std::uint64_t qwPreviewOfferID;
    std::uint32_t dwOfferNameLength;
    char* wszOfferName;
    std::uint32_t dwOfferType;
    std::uint8_t contentId[XMARKETPLACE_CONTENT_ID_LEN];
    bool fIsUnrestrictedLicense;
    std::uint32_t dwLicenseMask;
    std::uint32_t dwTitleID;
    std::uint32_t dwContentCategory;
    std::uint32_t dwTitleNameLength;
    char* wszTitleName;
    bool fUserHasPurchased;
    std::uint32_t dwPackageSize;
    std::uint32_t dwInstallSize;
    std::uint32_t dwSellTextLength;
    char* wszSellText;
    std::uint32_t dwAssetID;
    std::uint32_t dwPurchaseQuantity;
    std::uint32_t dwPointsPrice;
};
using PXMARKETPLACE_CONTENTOFFER_INFO = XMARKETPLACE_CONTENTOFFER_INFO*;

inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_CONTENT = 0x00000002;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_GAME_DEMO = 0x00000020;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_GAME_TRAILER = 0x00000040;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_THEME = 0x00000080;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_TILE = 0x00000800;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_ARCADE = 0x00002000;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_VIDEO = 0x00004000;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_CONSUMABLE = 0x00010000;
inline constexpr std::uint32_t XMARKETPLACE_OFFERING_TYPE_AVATARITEM = 0x00100000;
