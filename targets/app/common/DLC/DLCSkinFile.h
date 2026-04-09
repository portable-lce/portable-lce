#pragma once
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "DLCFile.h"
#include "app/common/DLC/DLCManager.h"
#include "minecraft/client/model/HumanoidModel.h"
#include "minecraft/client/model/SkinBox.h"
#include "minecraft/client/skins/ISkinAssetData.h"

class DLCSkinFile : public DLCFile, public ISkinAssetData {
private:
    std::string m_displayName;
    std::string m_themeName;
    std::string m_cape;
    unsigned int m_uiAnimOverrideBitmask;
    bool m_bIsFree;
    std::vector<SKIN_BOX*> m_AdditionalBoxes;

public:
    DLCSkinFile(const std::string& path);

    void addData(std::uint8_t* pbData, std::uint32_t dataBytes) override;
    void addParameter(DLCManager::EDLCParameterType type,
                      const std::string& value) override;

    std::string getParameterAsString(
        DLCManager::EDLCParameterType type) override;
    bool getParameterAsBool(DLCManager::EDLCParameterType type) override;

    // ISkinAssetData
    [[nodiscard]] std::uint32_t getSkinID() override {
        return DLCFile::getSkinID();
    }
    [[nodiscard]] unsigned int getAnimOverrideBitmask() override {
        return m_uiAnimOverrideBitmask;
    }
    [[nodiscard]] int getAdditionalBoxesCount() override;
    [[nodiscard]] std::vector<SKIN_BOX*>* getAdditionalBoxes() override;

    bool isFree() { return m_bIsFree; }
};
