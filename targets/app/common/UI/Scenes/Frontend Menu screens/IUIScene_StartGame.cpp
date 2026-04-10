#include "IUIScene_StartGame.h"

#include <wchar.h>

#include <cstdint>

#include "app/common/App_structs.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/Game.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/Controls/UIControl_BitmapIcon.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/Controls/UIControl_TexturePackList.h"
#include "app/common/UI/UIScene.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "platform/profile/profile.h"

class UILayer;

IUIScene_StartGame::IUIScene_StartGame(int iPad, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    m_bIgnoreInput = false;
    m_iTexturePacksNotInstalled = 0;
    m_texturePackDescDisplayed = false;
    m_bShowTexturePackDescription = false;
    m_iSetTexturePackDescription = -1;

    Minecraft* pMinecraft = Minecraft::GetInstance();
    m_currentTexturePackIndex = pMinecraft->skins->getTexturePackIndex(0);
}

void IUIScene_StartGame::HandleDLCMountingComplete() {
    Minecraft* pMinecraft = Minecraft::GetInstance();
    // clear out the current texture pack list
    m_texturePackList.clearSlots();

    int texturePacksCount = pMinecraft->skins->getTexturePackCount();

    for (unsigned int i = 0; i < texturePacksCount; ++i) {
        TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(i);

        std::uint32_t imageBytes = 0;
        std::uint8_t* imageData = tp->getPackIcon(imageBytes);

        if (imageBytes > 0 && imageData) {
            char imageName[64];
            snprintf(imageName, 64, "tpack%08x", tp->getId());
            registerSubstitutionTexture(imageName, imageData, imageBytes);
            m_texturePackList.addPack(i, imageName);
        }
    }

    m_iTexturePacksNotInstalled = 0;

    // 4J-PB - there may be texture packs we don't have, so use the info from
    // TMS for this REMOVE UNTIL WORKING
    DLC_INFO* pDLCInfo = nullptr;

    // first pass - look to see if there are any that are not in the list
    bool bTexturePackAlreadyListed;
    bool bNeedToGetTPD = false;

    for (unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount(); ++i) {
        bTexturePackAlreadyListed = false;
        uint64_t ull = app.GetDLCInfoTexturesFullOffer(i);
        pDLCInfo = app.GetDLCInfoForFullOfferID(ull);
        for (unsigned int i = 0; i < texturePacksCount; ++i) {
            TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(i);
            if (pDLCInfo->iConfig == tp->getDLCParentPackId()) {
                bTexturePackAlreadyListed = true;
            }
        }
        if (bTexturePackAlreadyListed == false) {
            // some missing
            bNeedToGetTPD = true;

            m_iTexturePacksNotInstalled++;
        }
    }

#if TO_BE_IMPLEMENTED
    if (bNeedToGetTPD == true) {
        // add a TMS request for them
        app.DebugPrintf("+++ Adding TMSPP request for texture pack data\n");
        app.AddTMSPPFileTypeRequest(e_DLC_TexturePackData);
        if (m_iConfigA != nullptr) {
            delete m_iConfigA;
        }
        m_iConfigA = new int[m_iTexturePacksNotInstalled];
        m_iTexturePacksNotInstalled = 0;

        for (unsigned int i = 0; i < app.GetDLCInfoTexturesOffersCount(); ++i) {
            bTexturePackAlreadyListed = false;
            uint64_t ull = app.GetDLCInfoTexturesFullOffer(i);
            pDLCInfo = app.GetDLCInfoForFullOfferID(ull);
            for (unsigned int i = 0; i < texturePacksCount; ++i) {
                TexturePack* tp = pMinecraft->skins->getTexturePackByIndex(i);
                if (pDLCInfo->iConfig == tp->getDLCParentPackId()) {
                    bTexturePackAlreadyListed = true;
                }
            }
            if (bTexturePackAlreadyListed == false) {
                m_iConfigA[m_iTexturePacksNotInstalled++] = pDLCInfo->iConfig;
            }
        }
    }
#endif
    m_currentTexturePackIndex = pMinecraft->skins->getTexturePackIndex(0);
    UpdateTexturePackDescription(m_currentTexturePackIndex);

    m_texturePackList.selectSlot(m_currentTexturePackIndex);
    m_bIgnoreInput = false;
    app.m_dlcManager.checkForCorruptDLCAndAlert();
}

void IUIScene_StartGame::handleSelectionChanged(F64 selectedId) {
    m_iSetTexturePackDescription = (int)selectedId;

    if (!m_texturePackDescDisplayed) {
        m_bShowTexturePackDescription = true;
    }
}

void IUIScene_StartGame::UpdateTexturePackDescription(int index) {
    TexturePack* tp =
        Minecraft::GetInstance()->skins->getTexturePackByIndex(index);

    if (tp == nullptr) {
#if TO_BE_IMPLEMENTED
        // this is probably a texture pack icon added from TMS

        unsigned int dwBytes = 0;
        unsigned int dwFileBytes = 0;
        std::uint8_t* pbData = nullptr;
        std::uint8_t* pbFileData = nullptr;

        CXuiCtrl4JList::LIST_ITEM_INFO ListItem;
        // get the current index of the list, and then get the data
        ListItem = m_pTexturePacksList->GetData(index);

        app.GetTPD(ListItem.iData, &pbData, &dwBytes);

        app.GetFileFromTPD(eTPDFileType_Loc, pbData, dwBytes, &pbFileData,
                           &dwFileBytes);
        if (dwFileBytes > 0 && pbFileData) {
            StringTable* pStringTable = new StringTable(
                std::span<const std::uint8_t>(pbFileData, dwFileBytes),
                app.getLocale());
            m_texturePackTitle.SetText(
                pStringTable->getString("IDS_DISPLAY_NAME"));
            m_texturePackDescription.SetText(
                pStringTable->getString("IDS_TP_DESCRIPTION"));
        }

        app.GetFileFromTPD(eTPDFileType_Icon, pbData, dwBytes, &pbFileData,
                           &dwFileBytes);
        if (dwFileBytes > 0 && pbFileData) {
            XuiCreateTextureBrushFromMemory(pbFileData, dwFileBytes,
                                            &m_hTexturePackIconBrush);
            m_texturePackIcon->UseBrush(m_hTexturePackIconBrush);
        }
        app.GetFileFromTPD(eTPDFileType_Comparison, pbData, dwBytes,
                           &pbFileData, &dwFileBytes);
        if (dwFileBytes > 0 && pbFileData) {
            XuiCreateTextureBrushFromMemory(pbFileData, dwFileBytes,
                                            &m_hTexturePackComparisonBrush);
            m_texturePackComparison->UseBrush(m_hTexturePackComparisonBrush);
        } else {
            m_texturePackComparison->UseBrush(nullptr);
        }
#endif
    } else {
        m_labelTexturePackName.setLabel(tp->getName());
        m_labelTexturePackDescription.setLabel(tp->getDesc1());

        std::uint32_t imageBytes = 0;
        std::uint8_t* imageData = tp->getPackIcon(imageBytes);

        // if(imageBytes > 0 && imageData)
        //{
        //	registerSubstitutionTexture("texturePackIcon", imageData,
        // imageBytes);
        //	m_bitmapTexturePackIcon.setTextureName("texturePackIcon");
        // }

        char imageName[64];
        snprintf(imageName, 64, "tpack%08x", tp->getId());
        m_bitmapTexturePackIcon.setTextureName(imageName);

        imageData = tp->getPackComparison(imageBytes);

        if (imageBytes > 0 && imageData) {
            snprintf(imageName, 64, "texturePackComparison%08x", tp->getId());
            registerSubstitutionTexture(imageName, imageData, imageBytes);
            m_bitmapComparison.setTextureName(imageName);
        } else {
            m_bitmapComparison.setTextureName("");
        }
    }
}

void IUIScene_StartGame::UpdateCurrentTexturePack(int iSlot) {
    m_currentTexturePackIndex = iSlot;
    TexturePack* tp = Minecraft::GetInstance()->skins->getTexturePackByIndex(
        m_currentTexturePackIndex);

    // if the texture pack is null, you don't have it yet
    if (tp == nullptr) {
#if TO_BE_IMPLEMENTED
        // Upsell

        CXuiCtrl4JList::LIST_ITEM_INFO ListItem;
        // get the current index of the list, and then get the data
        ListItem = m_pTexturePacksList->GetData(m_currentTexturePackIndex);

        // upsell the texture pack
        // tell sentient about the upsell of the full version of the skin pack
        uint64_t ullOfferID_Full;
        app.GetDLCFullOfferIDForPackID(ListItem.iData, &ullOfferID_Full);

        unsigned int uiIDA[3];

        uiIDA[0] = IDS_TEXTUREPACK_FULLVERSION;
        uiIDA[1] = IDS_TEXTURE_PACK_TRIALVERSION;
        uiIDA[2] = IDS_CONFIRM_CANCEL;

        // Give the player a warning about the texture pack missing
        ui.RequestErrorMessage(IDS_DLC_TEXTUREPACK_NOT_PRESENT_TITLE,
                               IDS_DLC_TEXTUREPACK_NOT_PRESENT, uiIDA, 3,
                               PlatformProfile.GetPrimaryPad(),
                               & : TexturePackDialogReturned, this);

        // do set the texture pack id, and on the user pressing create world,
        // check they have it
        m_MoreOptionsParams.dwTexturePack = ListItem.iData;
        return;
#endif
    } else {
        m_MoreOptionsParams.dwTexturePack = tp->getId();
    }
}

int IUIScene_StartGame::TrialTexturePackWarningReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_StartGame* pScene = (IUIScene_StartGame*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        pScene->checkStateAndStartGame();
    } else {
        pScene->m_bIgnoreInput = false;
    }
    return 0;
}

int IUIScene_StartGame::UnlockTexturePackReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_StartGame* pScene = (IUIScene_StartGame*)pParam;

    if (result == IPlatformStorage::EMessage_ResultAccept) {
        if (PlatformProfile.IsSignedIn(iPad)) {
            // the license change coming in when the offer has been installed
            // will cause this scene to refresh
        }
    } else {
    }

    pScene->m_bIgnoreInput = false;

    return 0;
}

int IUIScene_StartGame::TexturePackDialogReturned(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    IUIScene_StartGame* pClass = (IUIScene_StartGame*)pParam;

    pClass->m_bIgnoreInput = false;
    return 0;
}
