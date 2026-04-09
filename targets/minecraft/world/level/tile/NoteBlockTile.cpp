#include "NoteBlockTile.h"

#include <cmath>
#include <memory>

#include "minecraft/core/particles/ParticleTypes.h"
#include "app/common/Audio/SoundTypes.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/material/Material.h"
#include "minecraft/world/level/tile/BaseEntityTile.h"
#include "minecraft/world/level/tile/entity/MusicTileEntity.h"

NoteBlockTile::NoteBlockTile(int id) : BaseEntityTile(id, Material::wood) {}

void NoteBlockTile::neighborChanged(Level* level, int x, int y, int z,
                                    int type) {
    Log::info("-------- Neighbour changed type %d\n", type);
    bool signal = level->hasNeighborSignal(x, y, z);
    std::shared_ptr<MusicTileEntity> mte =
        std::dynamic_pointer_cast<MusicTileEntity>(
            level->getTileEntity(x, y, z));
    Log::info("-------- Signal is %s, tile is currently %s\n",
              signal ? "true" : "false", mte->on ? "ON" : "OFF");
    if (mte != nullptr && mte->on != signal) {
        if (signal) {
            mte->playNote(level, x, y, z);
        }
        mte->on = signal;
    }
}

// 4J-PB - Adding a TestUse for tooltip display
bool NoteBlockTile::TestUse() { return true; }

bool NoteBlockTile::use(Level* level, int x, int y, int z,
                        std::shared_ptr<Player> player, int clickedFace,
                        float clickX, float clickY, float clickZ,
                        bool soundOnly /*=false*/)  // 4J added soundOnly param
{
    if (soundOnly) return false;
    if (level->isClientSide) return true;
    std::shared_ptr<MusicTileEntity> mte =
        std::dynamic_pointer_cast<MusicTileEntity>(
            level->getTileEntity(x, y, z));
    if (mte != nullptr) {
        mte->tune();
        mte->playNote(level, x, y, z);
    }
    return true;
}

void NoteBlockTile::attack(Level* level, int x, int y, int z,
                           std::shared_ptr<Player> player) {
    if (level->isClientSide) return;
    std::shared_ptr<MusicTileEntity> mte =
        std::dynamic_pointer_cast<MusicTileEntity>(
            level->getTileEntity(x, y, z));
    if (mte != nullptr) mte->playNote(level, x, y, z);
}

std::shared_ptr<TileEntity> NoteBlockTile::newTileEntity(Level* level) {
    return std::make_shared<MusicTileEntity>();
}

bool NoteBlockTile::triggerEvent(Level* level, int x, int y, int z, int i,
                                 int note) {
    float pitch = (float)pow(2, (note - 12) / 12.0);

    int iSound;
    switch (i) {
        case 1:
            iSound = eSoundType_NOTE_BD;
            break;
        case 2:
            iSound = eSoundType_NOTE_SNARE;
            break;
        case 3:
            iSound = eSoundType_NOTE_HAT;
            break;
        case 4:
            iSound = eSoundType_NOTE_BASSATTACK;
            break;
        default:
            iSound = eSoundType_NOTE_HARP;
            break;
    }
    Log::info("NoteBlockTile::triggerEvent - playSound - pitch = %f\n", pitch);
    level->playSound(x + 0.5, y + 0.5, z + 0.5, iSound, 3, pitch);
    level->addParticle(eParticleType_note, x + 0.5, y + 1.2, z + 0.5,
                       note / 24.0, 0, 0);

    return true;
}
