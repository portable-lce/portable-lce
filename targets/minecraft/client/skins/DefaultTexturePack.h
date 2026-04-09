#pragma once
#include <string>

#include "AbstractTexturePack.h"
#include "java/InputOutputStream/InputStream.h"
#include "minecraft/IGameServices.h"
#include "strings.h"

class DefaultTexturePack : public AbstractTexturePack {
public:
    DefaultTexturePack();
    DLCPack* getDLCPack() { return nullptr; }

protected:
    //@Override
    void loadIcon();
    void loadName();
    void loadDescription();

public:
    //@Override
    bool hasFile(const std::string& name);
    bool isTerrainUpdateCompatible();

    std::string getDesc1() {
        return gameServices().getString(IDS_DEFAULT_TEXTUREPACK);
    }

protected:
    //@Override
    InputStream* getResourceImplementation(
        const std::string& name);  // throws FileNotFoundException

public:
    virtual bool hasData() { return true; }
    virtual bool hasAudio() { return false; }
    virtual bool isLoadingData() { return false; }
    virtual void loadUI();
    virtual void unloadUI();
};