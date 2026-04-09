#include "minecraft/IGameServices.h"

#include <cassert>

static IGameServices* s_services = nullptr;

void initGameServices(IGameServices* services) { s_services = services; }

IGameServices& gameServices() {
    assert(s_services &&
           "initGameServices() must be called before gameServices()");
    return *s_services;
}
