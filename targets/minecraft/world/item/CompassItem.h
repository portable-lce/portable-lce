#pragma once
// 4J Added so that we can override the icon id used to calculate the texture
// UV's for each player

#include <string>

#include "Item.h"
#include "platform/PlatformTypes.h"

class Icon;

class CompassItem : public Item {
private:
    Icon** icons;
    static const std::string TEXTURE_PLAYER_ICON[XUSER_MAX_COUNT];

public:
    CompassItem(int id);

    virtual Icon* getIcon(int auxValue);

    //@Override
    void registerIcons(IconRegister* iconRegister);
};