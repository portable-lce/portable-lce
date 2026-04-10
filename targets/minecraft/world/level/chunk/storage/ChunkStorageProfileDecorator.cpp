#include "ChunkStorageProfileDecorator.h"

#include <stdio.h>

#include "java/System.h"
#include "minecraft/util/Log.h"
#include "minecraft/world/level/chunk/storage/ChunkStorage.h"

ChunkStorageProfilerDecorator::ChunkStorageProfilerDecorator(
    ChunkStorage* capsulated)
    : timeSpentLoading(0),
      loadCount(0),
      timeSpentSaving(0),
      saveCount(0),
      counter(0) {
    this->capsulated = capsulated;
}

LevelChunk* ChunkStorageProfilerDecorator::load(Level* level, int x, int z) {
    int64_t nanoTime = System::nanoTime();
    LevelChunk* chunk = capsulated->load(level, x, z);
    timeSpentLoading += System::nanoTime() - nanoTime;
    loadCount++;

    return chunk;
}

void ChunkStorageProfilerDecorator::save(Level* level, LevelChunk* levelChunk) {
    int64_t nanoTime = System::nanoTime();
    capsulated->save(level, levelChunk);
    timeSpentSaving += System::nanoTime() - nanoTime;
    saveCount++;
}

void ChunkStorageProfilerDecorator::saveEntities(Level* level,
                                                 LevelChunk* levelChunk) {
    capsulated->saveEntities(level, levelChunk);
}

void ChunkStorageProfilerDecorator::tick() {
    char buf[256];
    capsulated->tick();

    counter++;
    if (counter > 500) {
        if (loadCount > 0) {
#if !defined(_CONTENT_PACKAGE)
            sprintf(buf, "Average load time: %f (%lld)",
                    0.000001 * (double)timeSpentLoading / (double)loadCount,
                    (long long)loadCount);
            Log::info("%s", buf);
#endif
        }
        if (saveCount > 0) {
#if !defined(_CONTENT_PACKAGE)
            sprintf(buf, "Average save time: %f (%lld)",
                    0.000001 * (double)timeSpentSaving / (double)loadCount,
                    (long long)loadCount);
            Log::info("%s", buf);
#endif
        }
        counter = 0;
    }
}

void ChunkStorageProfilerDecorator::flush() { capsulated->flush(); }