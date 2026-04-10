#include "UIComponent_DebugUIMarketingGuide.h"

#include "app/common/UI/UIScene.h"
#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"

class UILayer;

UIComponent_DebugUIMarketingGuide::UIComponent_DebugUIMarketingGuide(
    int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer) {
    // Setup all the Iggy references we need for this scene
    initialiseMovie();

    IggyDataValue result;
    IggyDataValue value[1];
    value[0].type = IGGY_DATATYPE_number;
    value[0].number = (F64)0;  // WIN64
#if defined(_WINDOWS64) || defined(__linux__)
    value[0].number = (F64)0;
#endif
    IggyResult out = IggyPlayerCallMethodRS(getMovie(), &result,
                                            IggyPlayerRootPath(getMovie()),
                                            m_funcSetPlatform, 1, value);
}

std::string UIComponent_DebugUIMarketingGuide::getMoviePath() {
    return "DebugUIMarketingGuide";
}
