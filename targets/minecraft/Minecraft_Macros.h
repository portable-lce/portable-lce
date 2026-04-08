
#pragma once

// 3 bit user index
// 5 bits alpha
// 1 bit decoration
// 3 bits poptime
// 8 bits unused // was 11 bits aux val but needed 15 bits for potions so moved
// to item bitmask 6 bits count 6 bits scale

// uiCount is up to 64, but can't ever be 0, so to make it 6 bits, subtract one
// from the packing, and add one on the unpacking
#define MAKE_SLOTDISPLAY_DATA_BITMASK(uiUserIndex, uiAlpha, bDecorations, \
                                      uiCount, uiScale, uiPopTime)        \
    ((((uiUserIndex & 0x7) << 29) | (uiAlpha & 0x1F) << 24) |             \
     (bDecorations ? 0x800000 : 0) | ((uiPopTime & 0x7) << 20) |          \
     ((uiCount - 1) << 6) | (uiScale & 0x3F))

#define GET_SLOTDISPLAY_USERINDEX_FROM_DATA_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) >> 29) & 0x7)
#define GET_SLOTDISPLAY_ALPHA_FROM_DATA_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) >> 24) & 0x1F)
#define GET_SLOTDISPLAY_DECORATIONS_FROM_DATA_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) & 0x800000) ? true : false)
// #define GET_SLOTDISPLAY_AUXVAL_FROM_DATA_BITMASK(uiBitmask)
// ((((unsigned long)uiBitmask)>>12)&0x7FF)
#define GET_SLOTDISPLAY_COUNT_FROM_DATA_BITMASK(uiBitmask) \
    (((((unsigned int)uiBitmask) >> 6) & 0x3F) + 1)
#define GET_SLOTDISPLAY_SCALE_FROM_DATA_BITMASK(uiBitmask) \
    (((unsigned int)uiBitmask) & 0x3F)
#define GET_SLOTDISPLAY_POPTIME_FROM_DATA_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) >> 20) & 0x7)

// 16 bits for id (either item id or xzp icon id)
// 15 bits for aux value
// 1 bit for foil
#define MAKE_SLOTDISPLAY_ITEM_BITMASK(uiId, uiAuxValue, bFoil) \
    ((uiId & 0xFFFF) | ((uiAuxValue & 0x7FFF) << 16) | (bFoil ? 0x80000000 : 0))

#define GET_SLOTDISPLAY_ID_FROM_ITEM_BITMASK(uiBitmask) \
    (((unsigned int)uiBitmask) & 0xFFFF)
#define GET_SLOTDISPLAY_AUXVAL_FROM_ITEM_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) >> 16) & 0x7FFF)
#define GET_SLOTDISPLAY_FOIL_FROM_ITEM_BITMASK(uiBitmask) \
    ((((unsigned int)uiBitmask) & 0x80000000) ? true : false)

// For encoding the players skin selection in their profile
// bDlcSkin = false is a players skin, bDlcSkin = true is a DLC skin
#define MAKE_SKIN_BITMASK(bDlcSkin, dwSkinId) \
    ((bDlcSkin ? 0x80000000 : 0) | (dwSkinId & 0x7FFFFFFF))
#define IS_SKIN_ID_IN_RANGE(dwSkinId) (dwSkinId <= 0x7FFFFFFF)

#define GET_DLC_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (((uint32_t)uiBitmask) & 0x7FFFFFFF)
#define GET_UGC_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (((uint32_t)uiBitmask) & 0x7FFFFFE0)
#define GET_DEFAULT_SKIN_ID_FROM_BITMASK(uiBitmask) \
    (((uint32_t)uiBitmask) & 0x0000001F)
#define GET_IS_DLC_SKIN_FROM_BITMASK(uiBitmask) \
    ((((uint32_t)uiBitmask) & 0x80000000) ? true : false)
