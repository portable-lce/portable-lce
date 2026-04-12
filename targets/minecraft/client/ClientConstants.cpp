#include "ClientConstants.h"

#include "minecraft/BuildVer.h"

const std::string ClientConstants::VERSION_STRING =
    std::string("Minecraft Xbox ") + VER_FILEVERSION_STR_W +
    std::string(" (Portable LCE)");  //+ SharedConstants::VERSION_STRING;
