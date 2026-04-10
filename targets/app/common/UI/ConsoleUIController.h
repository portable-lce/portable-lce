#pragma once

#include "app/common/Iggy/include/iggy.h"
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/UIController.h"
#include "platform/profile/profile.h"

class ConsoleUIController : public UIController {
public:
    void init(S32 w, S32 h);

    void render();

    void shutdown();
    void beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion* region,
                               CustomDrawData* customDrawRegion);
    virtual CustomDrawData* setupCustomDraw(
        UIScene* scene, IggyCustomDrawCallbackRegion* region);
    virtual CustomDrawData* calculateCustomDraw(
        IggyCustomDrawCallbackRegion* region);
    virtual void endCustomDraw(IggyCustomDrawCallbackRegion* region);

protected:
    virtual void setTileOrigin(S32 xPos, S32 yPos);

public:
    GDrawTexture* getSubstitutionTexture(int textureId);
    void destroySubstitutionTexture(void* destroyCallBackData,
                                    GDrawTexture* handle);

    static void handleUnlockFullVersionCallback();
};

extern ConsoleUIController ui;