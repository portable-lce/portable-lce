#pragma once
#include <cstdint>
#include <vector>

#include "minecraft/client/model/SkinBox.h"

// Domain-shaped view of a custom skin asset.
//
// minecraft/ consumers (Player, the client/server connection layers,
// the texture packet) need a handful of fields off a custom skin
// asset: the skin id, the additional body parts geometry, and an
// animation override mask. Historically those fields were read off
// app/common/DLC/DLCSkinFile, which dragged the entire DLC subsystem
// up into minecraft/ headers and broke the layering invariant.
//
// This interface is the domain abstraction. The DLC implementation in
// app/ inherits from it; minecraft/ only ever sees ISkinAssetData.
class ISkinAssetData {
public:
    virtual ~ISkinAssetData() = default;

    [[nodiscard]] virtual std::uint32_t getSkinID() = 0;
    [[nodiscard]] virtual unsigned int getAnimOverrideBitmask() = 0;
    [[nodiscard]] virtual int getAdditionalBoxesCount() = 0;
    [[nodiscard]] virtual std::vector<SKIN_BOX*>* getAdditionalBoxes() = 0;
};
