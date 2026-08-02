
#include "UIScene_DLCOffersMenu.h"

#include <stdint.h>

#include "app/common/Game.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/ConsoleUIController.h"
#include "app/common/UI/Controls/UIControl_DLCList.h"
#include "app/common/UI/Controls/UIControl_HTMLLabel.h"
#include "app/common/UI/Controls/UIControl_Label.h"
#include "app/common/UI/UIScene.h"
#include "platform/PlatformTypes.h"
#include "platform/renderer/renderer.h"
#include "strings.h"
#include "platform/renderer/IRenderPath.h"


class UILayer;

#define PLAYER_ONLINE_TIMER_ID 0
#define PLAYER_ONLINE_TIMER_TIME 100

UIScene_DLCOffersMenu::UIScene_DLCOffersMenu(int iPad, void* initData,
                                             UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    m_bProductInfoShown = false;
    DLCOffersParam* param = (DLCOffersParam*)initData;
    m_iProductInfoIndex = param->iType;
    m_iCurrentDLC = 0;
    m_iTotalDLC = 0;
    m_bAddAllDLCButtons = true;

    // Setup all the Iggy references we need for this scene
    initialiseMovie();
    // Alert the app the we want to be informed of ethernet connections
    app.SetLiveLinkRequired(true);

    m_bIsSD = !RenderPath.framebuffer().is_hi_def && !RenderPath.framebuffer().is_widescreen;

    m_labelOffers.init(app.GetString(IDS_DOWNLOADABLE_CONTENT_OFFERS));
    m_buttonListOffers.init(eControl_OffersList);
    m_labelHTMLSellText.init(" ");
    m_labelPriceTag.init(" ");

    m_bHasPurchased = false;
    m_bIsSelected = false;

    if (m_loadedResolution == eSceneResolution_1080) {
        m_labelXboxStore.init("");
    }
}

UIScene_DLCOffersMenu::~UIScene_DLCOffersMenu() {
    // Alert the app the we no longer want to be informed of ethernet
    // connections
    app.SetLiveLinkRequired(false);
}

void UIScene_DLCOffersMenu::handleTimerComplete(int id) {}

int UIScene_DLCOffersMenu::ExitDLCOffersMenu(
    void* pParam, int iPad, IPlatformStorage::EMessageResult result) {
    UIScene_DLCOffersMenu* pClass = (UIScene_DLCOffersMenu*)pParam;

    ui.NavigateToHomeMenu();  // iPad,eUIScene_MainMenu);

    return 0;
}

std::string UIScene_DLCOffersMenu::getMoviePath() { return "DLCOffersMenu"; }

void UIScene_DLCOffersMenu::updateTooltips() {
    int iA = -1;
    if (m_bIsSelected) {
        if (!m_bHasPurchased) {
            iA = IDS_TOOLTIPS_INSTALL;
        } else {
            iA = IDS_TOOLTIPS_REINSTALL;
        }
    }
    ui.SetTooltips(m_iPad, iA, IDS_TOOLTIPS_BACK);
}

void UIScene_DLCOffersMenu::handleInput(int iPad, int key, bool repeat,
                                        bool pressed, bool released,
                                        bool& handled) {
    // app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d,
    // down- %s, pressed- %s, released- %s\n", iPad, key, down?"true":"false",
    // pressed?"true":"false", released?"true":"false");
    ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

    switch (key) {
        case ACTION_MENU_CANCEL:
            if (pressed) {
                navigateBack();
            }
            break;
        case ACTION_MENU_OK:
            sendInputToMovie(key, repeat, pressed, released);
            break;
        case ACTION_MENU_UP:
            if (pressed) {
                // 4J - TomK don't proceed if there is no DLC to navigate
                // through
                if (m_iTotalDLC > 0) {
                    if (m_iCurrentDLC > 0) m_iCurrentDLC--;

                    m_bProductInfoShown = false;
                }
            }
            sendInputToMovie(key, repeat, pressed, released);
            break;

        case ACTION_MENU_DOWN:
            if (pressed) {
                // 4J - TomK don't proceed if there is no DLC to navigate
                // through
                if (m_iTotalDLC > 0) {
                    if (m_iCurrentDLC < (m_iTotalDLC - 1)) m_iCurrentDLC++;

                    m_bProductInfoShown = false;
                }
            }
            sendInputToMovie(key, repeat, pressed, released);
            break;

        case ACTION_MENU_LEFT:
            /*
#if defined(_DEBUG)
    static int iTextC=0;
    switch(iTextC)
    {
    case 0:
            m_labelHTMLSellText.init("Voici un fantastique mini-pack de 24
apparences pour personnaliser votre personnage Minecraft et vous mettre dans
l'ambiance des f�tes de fin d'ann�e.<br><br>1-4 joueurs<br>2-8 joueurs en
r�seau<br><br>  Cet article fait l�objet d�une licence ou d�une sous-licence de
Sony Computer Entertainment America, et est soumis aux conditions g�n�rales du
service du r�seau, au contrat d�utilisateur, aux restrictions d�utilisation de
cet article et aux autres conditions applicables, disponibles sur le site
www.us.playstation.com/support/useragreements. Si vous ne souhaitez pas accepter
ces conditions, ne t�l�chargez pas ce produit. Cet article peut �tre utilis�
avec un maximum de deux syst�mes PlayStation�3 activ�s associ�s � ce compte Sony
Entertainment Network.�<br><br>'Minecraft' est une marque commerciale de Notch
Development AB."); break; case 1: m_labelHTMLSellText.init("Un fabuloso minipack
de 24 aspectos para personalizar tu personaje de Minecraft y ponerte a tono con
las fiestas.<br><br>1-4 jugadores<br>2-8 jugadores en red<br><br>  Sony Computer
Entertainment America le concede la licencia o sublicencia de este art�culo, que
est� sujeto a los t�rminos de servicio y al acuerdo de usuario de la red. Las
restricciones de uso de este art�culo, as� como otros t�rminos aplicables, se
encuentran en www.us.playstation.com/support/useragreements. Si no desea aceptar
todos estos t�rminos, no descargue este art�culo. Este art�culo puede usarse en
hasta dos sistemas PlayStation�3 activados asociados con esta cuenta de Sony
Entertainment Network.�<br><br>'Minecraft' es una marca comercial de Notch
Development AB."); break; case 2: m_labelHTMLSellText.init("Este � um incr�vel
pacote com 24 capas para personalizar seu personagem no Minecraft e entrar no
clima de final de ano.<br><br>1-4 Jogadores<br>Jogadores em rede 2-8<br><br>
Este item est� sendo licenciado ou sublicenciado para voc� pela Sony Computer
Entertainment America e est� sujeito aos Termos de Servi�o da Rede e Acordo do
Usu�rio, as restri��es de uso deste item e outros termos aplic�veis est�o
localizados em www.us.playstation.com/support/useragreements. Caso n�o queira
aceitar todos esses termos, n�o baixe este item. Este item pode ser usado com
at� 2 sistemas PlayStation�3 ativados associados a esta Conta de Rede Sony
Entertainment.�<br><br>'Minecraft' � uma marca registrada da Notch Development
AB"); break;
    }
    iTextC++;
    if(iTextC>2) iTextC=0;
#endif
    */
        case ACTION_MENU_RIGHT:
        case ACTION_MENU_OTHER_STICK_DOWN:
        case ACTION_MENU_OTHER_STICK_UP:
            // don't pass down PageUp or PageDown because this will cause
            // conflicts between the buttonlist and scrollable html text
            // component
            // case ACTION_MENU_PAGEUP:
            // case ACTION_MENU_PAGEDOWN:
            sendInputToMovie(key, repeat, pressed, released);
            break;
    }
}

void UIScene_DLCOffersMenu::handlePress(F64 controlId, F64 childId) {
    switch ((int)controlId) {
        case eControl_OffersList: {
            int iIndex = (int)childId;

            uint64_t ullIndexA[1];
            ullIndexA[0] = PlatformStorage.GetOffer(iIndex).qwOfferID;
            PlatformStorage.InstallOffer(1, ullIndexA, nullptr);
        } break;
    }
}

void UIScene_DLCOffersMenu::handleSelectionChanged(F64 selectedId) {}

void UIScene_DLCOffersMenu::handleFocusChange(F64 controlId, F64 childId) {
    app.DebugPrintf("UIScene_DLCOffersMenu::handleFocusChange\n");
}

void UIScene_DLCOffersMenu::tick() { UIScene::tick(); }
