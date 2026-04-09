#pragma once
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "DLCFile.h"
#include "app/common/DLC/DLCManager.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/SkinBox.h"

class DLCSkinFile : public DLCFile {
private:
    std::string m_displayName;
    std::string m_themeName;
    std::string m_cape;
    unsigned int m_uiAnimOverrideBitmask;
    bool m_bIsFree;
    std::vector<SKIN_BOX*> m_AdditionalBoxes;

public:
    DLCSkinFile(const std::string& path);

    virtual void addData(std::uint8_t* pbData, std::uint32_t dataBytes);
    virtual void addParameter(DLCManager::EDLCParameterType type,
                              const std::string& value);

    virtual std::string getParameterAsString(
        DLCManager::EDLCParameterType type);
    virtual bool getParameterAsBool(DLCManager::EDLCParameterType type);
    std::vector<SKIN_BOX*>* getAdditionalBoxes();
    int getAdditionalBoxesCount();
    unsigned int getAnimOverrideBitmask() { return m_uiAnimOverrideBitmask; }
    bool isFree() { return m_bIsFree; }
};
