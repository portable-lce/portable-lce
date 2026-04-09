#pragma once

#include <vector>

#include "app/common/App_structs.h"
#include "minecraft/GameEnums.h"

class TerrainFeatureManager {
public:
    void add(_eTerrainFeatureType eFeatureType, int x, int z);
    void clear();
    _eTerrainFeatureType isFeature(int x, int z) const;
    bool getPosition(_eTerrainFeatureType eType, int* pX, int* pZ) const;

    std::vector<FEATURE_DATA*>* features() { return &m_vTerrainFeatures; }

private:
    std::vector<FEATURE_DATA*> m_vTerrainFeatures;
};
