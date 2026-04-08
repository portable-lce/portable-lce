
#include "minecraft/util/WeighedRandom.h"

#include <vector>
#include <cassert>

#include "java/Random.h"

int WeighedRandom::getTotalWeight(std::vector<WeighedRandomItem*>* items) {
    int totalWeight = 0;
    for (auto it = items->begin(); it != items->end(); it++) {
        totalWeight += (*it)->randomWeight;
    }
    return totalWeight;
}

WeighedRandomItem* WeighedRandom::getRandomItem(
    Random* random, std::vector<WeighedRandomItem*>* items, int totalWeight) {
    if (totalWeight <= 0) {
        assert(0);
    }

    int selection = random->nextInt(totalWeight);

    for (auto it = items->begin(); it != items->end(); it++) {
        selection -= (*it)->randomWeight;
        if (selection < 0) {
            return *it;
        }
    }
    return nullptr;
}

WeighedRandomItem* WeighedRandom::getRandomItem(
    Random* random, std::vector<WeighedRandomItem*>* items) {
    return getRandomItem(random, items, getTotalWeight(items));
}

int WeighedRandom::getTotalWeight(
    const std::vector<WeighedRandomItem*>& items) {
    int totalWeight = 0;
    for (unsigned int i = 0; i < items.size(); i++) {
        totalWeight += items[i]->randomWeight;
    }
    return totalWeight;
}

WeighedRandomItem* WeighedRandom::getRandomItem(
    Random* random, const std::vector<WeighedRandomItem*>& items,
    int totalWeight) {
    if (totalWeight <= 0) {
        assert(0);
    }

    int selection = random->nextInt(totalWeight);
    for (unsigned int i = 0; i < items.size(); i++) {
        selection -= items[i]->randomWeight;
        if (selection < 0) {
            return items[i];
        }
    }
    return nullptr;
}

WeighedRandomItem* WeighedRandom::getRandomItem(
    Random* random, const std::vector<WeighedRandomItem*>& items) {
    return getRandomItem(random, items, getTotalWeight(items));
}