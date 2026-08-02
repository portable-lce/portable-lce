#include "TheEndBiomeDecorator.h"

#include <memory>
#include <numbers>

#include "java/Random.h"
#include "minecraft/world/entity/boss/enderdragon/EnderDragon.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/biome/BiomeDecorator.h"
#include "minecraft/world/level/levelgen/feature/EndPodiumFeature.h"
#include "minecraft/world/level/levelgen/feature/Feature.h"
#include "minecraft/world/level/levelgen/feature/SpikeFeature.h"
#include "minecraft/world/level/tile/Tile.h"

class Biome;

// Spike centre positions, calculated using
// for(int i=0;i<8;i++)
// {
// int x=40 * cosf(2*(-std::numbers::pi +(PI/8)*i));
// int z=40* sinf(2*(-std::numbers::pi +(PI/8)*i));
// }

TheEndBiomeDecorator::SPIKE TheEndBiomeDecorator::SpikeValA[8] = {
    // The chunk that the spike is in has to be the smallest x and z that any
    // part of it is in a chunk(x,z) will only be post-processed when the chunks
    // (x+1,z), (x,z+1) and (x+1,z+1) are also loaded

    // first two values are the smallest x and z of the chunk that the feature
    // is in - so the centre point minus the radius
    {32, -16, 40, 0, 2},      // smallest block - 38,-2
    {16, 16, 28, 28, 2},      // smallest block - 26,26
    {-16, 32, 0, 40, 2},      // smallest block - -2,38
    {-32, 16, -28, 28, 3},    // smallest block - -31,26
    {-48, -16, -40, 0, 3},    // smallest block - -43,-3
    {-32, -32, -28, -28, 3},  // smallest block - -31,-31
    {-16, -48, 0, -40, 4},    // smallest block - -4,-44
    {16, -32, 28, -28, 4},    // smallest block - 24,-32
};

TheEndBiomeDecorator::TheEndBiomeDecorator(Biome* biome)
    : BiomeDecorator(biome) {
    spikeFeature = new SpikeFeature(Tile::endStone_Id);
    endPodiumFeature = new EndPodiumFeature(Tile::endStone_Id);
}

void TheEndBiomeDecorator::decorate() {
    decorateOres();

    // this will only set the y to the top y of the chunks already processed...
    int y = level->getTopSolidBlock(xo + 8, zo + 8);
    if (y > level->GetHighestY()) level->SetHighestY(y);

    // 4J-PB - editing to place 8 spikes in a circle, with increasing height

    // are we within the chunk with a spike?
    for (int i = 0; i < 8; i++) {
        if ((xo == SpikeValA[i].iChunkX) && (zo == SpikeValA[i].iChunkZ)) {
            // in the right chunk
            spikeFeature->placeWithIndex(level, random, SpikeValA[i].x,
                                         level->GetHighestY(), SpikeValA[i].z,
                                         i, SpikeValA[i].radius);
        }
    }
    if (xo == 0 && zo == 0) {
        std::shared_ptr<EnderDragon> enderDragon =
            std::make_shared<EnderDragon>(level);
        enderDragon->AddParts();  // 4J added
        enderDragon->moveTo(0, 128, 0, random->nextFloat() * 360, 0);
        level->addEntity(enderDragon);
    }

    // end podium radius is 4, position is 0,0, so chunk needs to be the -16,-16
    // one since this guarantees that all chunks required for the podium are
    // loaded
    if (xo == -16 && zo == -16) {
        endPodiumFeature->place(level, random, 0, level->seaLevel, 0);
    }
}