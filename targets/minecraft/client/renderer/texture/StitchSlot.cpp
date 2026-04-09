#include "StitchSlot.h"

#include <algorithm>
#include <vector>

#include "TextureHolder.h"
#include "util/StringHelpers.h"

StitchSlot::StitchSlot(int originX, int originY, int width, int height)
    : originX(originX), originY(originY), width(width), height(height) {
    subSlots = nullptr;
    textureHolder = nullptr;
}

TextureHolder* StitchSlot::getHolder() { return textureHolder; }

int StitchSlot::getX() { return originX; }

int StitchSlot::getY() { return originY; }

bool StitchSlot::add(TextureHolder* textureHolder) {
    // Already holding a texture -- doesn't account for subslots.
    if (this->textureHolder != nullptr) {
        return false;
    }

    int textureWidth = textureHolder->getWidth();
    int textureHeight = textureHolder->getHeight();

    // We're too small to fit the texture
    if (textureWidth > width || textureHeight > height) {
        return false;
    }

    // Exact fit! best-case-solution
    if (textureWidth == width && textureHeight == height &&
        subSlots == nullptr) {
        // Store somehow
        this->textureHolder = textureHolder;
        return true;
    }

    // See if we're already divided before, if not, setup subSlots
    if (subSlots == nullptr) {
        subSlots = new std::vector<StitchSlot*>();

        // First slot is for the new texture
        subSlots->push_back(
            new StitchSlot(originX, originY, textureWidth, textureHeight));

        int spareWidth = width - textureWidth;
        int spareHeight = height - textureHeight;

        if (spareHeight > 0 && spareWidth > 0) {
            // Space below AND right
            //
            //       <-right->
            // +-----+-------+
            // |     |       |
            // | Tex |       |
            // |     |       |
            // |-----+       | ^
            // |             | |- bottom
            // +-------------+ v
            // We need to add two more areas, the one with the 'biggest'
            // dimensions should be used (In the case of this ASCII drawing,
            // it's the 'right hand side' that should win)

            // The 'fattest' area should be used (or when tied, the right hand
            // one)
            int right = std::max(height, spareWidth);
            int bottom = std::max(width, spareHeight);
            if (right >= bottom) {
                subSlots->push_back(new StitchSlot(originX,
                                                   originY + textureHeight,
                                                   textureWidth, spareHeight));
                subSlots->push_back(new StitchSlot(
                    originX + textureWidth, originY, spareWidth, height));
            } else {
                subSlots->push_back(new StitchSlot(originX + textureWidth,
                                                   originY, spareWidth,
                                                   textureHeight));
                subSlots->push_back(new StitchSlot(
                    originX, originY + textureHeight, width, spareHeight));
            }

        } else if (spareWidth == 0) {
            // We just have space left below
            //
            // +-------------+
            // |             |
            // | Tex         |
            // |             |
            // |-------------+ ^
            // |             | |- bottom
            // +-------------+ v
            subSlots->push_back(new StitchSlot(originX, originY + textureHeight,
                                               textureWidth, spareHeight));
        } else if (spareHeight == 0) {
            // Only space to the right
            //
            //       <-right->
            // +-----+-------+
            // |     |       |
            // | Tex |       |
            // |     |       |
            // |     |       |
            // |     |       |
            // +-----+-------+
            subSlots->push_back(new StitchSlot(originX + textureWidth, originY,
                                               spareWidth, textureHeight));
        }
    }

    // for (final StitchSlot subSlot : subSlots)
    for (auto it = subSlots->begin(); it != subSlots->end(); ++it) {
        StitchSlot* subSlot = *it;
        if (subSlot->add(textureHolder)) {
            return true;
        }
    }

    return false;
}

void StitchSlot::collectAssignments(std::vector<StitchSlot*>* result) {
    if (textureHolder != nullptr) {
        result->push_back(this);
    } else if (subSlots != nullptr) {
        // for (StitchSlot subSlot : subSlots)
        for (auto it = subSlots->begin(); it != subSlots->end(); ++it) {
            StitchSlot* subSlot = *it;
            subSlot->collectAssignments(result);
        }
    }
}

//@Override
std::string StitchSlot::toString() {
    return "Slot{originX=" + toWString(originX) +
           ", originY=" + toWString(originY) + ", width=" + toWString(width) +
           ", height=" + toWString(height) +
           ", texture=" + toWString(textureHolder) +
           ", subSlots=" + toWString(subSlots) + '}';
}