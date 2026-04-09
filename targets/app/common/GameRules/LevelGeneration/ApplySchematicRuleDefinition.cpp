#include "ApplySchematicRuleDefinition.h"

#include <algorithm>
#include <cmath>

#include "app/linux/LinuxGame.h"
#include "minecraft/world/level/levelgen/ConsoleSchematicFile.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "minecraft/world/level/GameRules/LevelGenerationOptions.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/dimension/Dimension.h"
#include "minecraft/world/phys/AABB.h"
#include "util/StringHelpers.h"

ApplySchematicRuleDefinition::ApplySchematicRuleDefinition(
    LevelGenerationOptions* levelGenOptions) {
    m_levelGenOptions = levelGenOptions;
    m_location = Vec3(0, 0, 0);
    m_locationBox = std::nullopt;
    m_totalBlocksChanged = 0;
    m_totalBlocksChangedLighting = 0;
    m_rotation = ConsoleSchematicFile::eSchematicRot_0;
    m_completed = false;
    m_dimension = 0;
    m_schematic = nullptr;
}

ApplySchematicRuleDefinition::~ApplySchematicRuleDefinition() {
    app.DebugPrintf("Deleting ApplySchematicRuleDefinition.\n");
    if (!m_completed) m_levelGenOptions->releaseSchematicFile(m_schematicName);
    m_schematic = nullptr;
}

void ApplySchematicRuleDefinition::writeAttributes(DataOutputStream* dos,
                                                   unsigned int numAttrs) {
    GameRuleDefinition::writeAttributes(dos, numAttrs + 5);

    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_filename);
    dos->writeUTF(m_schematicName);
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_x);
    dos->writeUTF(toWString(m_location.x));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_y);
    dos->writeUTF(toWString(m_location.y));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_z);
    dos->writeUTF(toWString(m_location.z));
    ConsoleGameRules::write(dos, ConsoleGameRules::eGameRuleAttr_rot);

    switch (m_rotation) {
        case ConsoleSchematicFile::eSchematicRot_0:
            dos->writeUTF(toWString(0));
            break;
        case ConsoleSchematicFile::eSchematicRot_90:
            dos->writeUTF(toWString(90));
            break;
        case ConsoleSchematicFile::eSchematicRot_180:
            dos->writeUTF(toWString(180));
            break;
        case ConsoleSchematicFile::eSchematicRot_270:
            dos->writeUTF(toWString(270));
            break;
    }
}

void ApplySchematicRuleDefinition::addAttribute(
    const std::string& attributeName, const std::string& attributeValue) {
    if (attributeName.compare("filename") == 0) {
        m_schematicName = attributeValue;
        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // filename=%s\n",m_schematicName.c_str());

        if (!m_schematicName.empty()) {
            if (m_schematicName
                    .substr(m_schematicName.length() - 4,
                            m_schematicName.length())
                    .compare(".sch") != 0) {
                m_schematicName.append(".sch");
            }
            m_schematic = m_levelGenOptions->getSchematicFile(m_schematicName);
        }
    } else if (attributeName.compare("x") == 0) {
        m_location.x = fromWString<int>(attributeValue);
        if (((int)std::abs(m_location.x)) % 2 != 0) m_location.x -= 1;
        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // x=%f\n",m_location->x);
    } else if (attributeName.compare("y") == 0) {
        m_location.y = fromWString<int>(attributeValue);
        if (((int)std::abs(m_location.y)) % 2 != 0) m_location.y -= 1;
        if (m_location.y < 0) m_location.y = 0;
        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // y=%f\n",m_location->y);
    } else if (attributeName.compare("z") == 0) {
        m_location.z = fromWString<int>(attributeValue);
        if (((int)std::abs(m_location.z)) % 2 != 0) m_location.z -= 1;
        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // z=%f\n",m_location->z);
    } else if (attributeName.compare("rot") == 0) {
        int degrees = fromWString<int>(attributeValue);

        while (degrees < 0) degrees += 360;
        while (degrees >= 360) degrees -= 360;
        float quad = degrees / 90;
        degrees = (int)(quad + 0.5f);
        switch (degrees) {
            case 1:
                m_rotation = ConsoleSchematicFile::eSchematicRot_90;
                break;
            case 2:
                m_rotation = ConsoleSchematicFile::eSchematicRot_180;
                break;
            case 3:
            case 4:
                m_rotation = ConsoleSchematicFile::eSchematicRot_270;
                break;
            case 0:
            default:
                m_rotation = ConsoleSchematicFile::eSchematicRot_0;
                break;
        };

        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // rot=%d\n",m_rotation);
    } else if (attributeName.compare("dim") == 0) {
        m_dimension = fromWString<int>(attributeValue);
        if (m_dimension > 1 || m_dimension < -1) m_dimension = 0;
        // app.DebugPrintf("ApplySchematicRuleDefinition: Adding parameter
        // dimension=%d\n",m_dimension);
    } else {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

void ApplySchematicRuleDefinition::updateLocationBox() {
    if (m_schematic == nullptr)
        m_schematic = m_levelGenOptions->getSchematicFile(m_schematicName);

    m_locationBox = AABB(0, 0, 0, 0, 0, 0);

    m_locationBox->x0 = m_location.x;
    m_locationBox->y0 = m_location.y;
    m_locationBox->z0 = m_location.z;

    m_locationBox->y1 = m_location.y + m_schematic->getYSize();

    switch (m_rotation) {
        case ConsoleSchematicFile::eSchematicRot_90:
        case ConsoleSchematicFile::eSchematicRot_270:
            m_locationBox->x1 = m_location.x + m_schematic->getZSize();
            m_locationBox->z1 = m_location.z + m_schematic->getXSize();
            break;
        case ConsoleSchematicFile::eSchematicRot_0:
        case ConsoleSchematicFile::eSchematicRot_180:
        default:
            m_locationBox->x1 = m_location.x + m_schematic->getXSize();
            m_locationBox->z1 = m_location.z + m_schematic->getZSize();
            break;
    };
}

void ApplySchematicRuleDefinition::processSchematic(AABB* chunkBox,
                                                    LevelChunk* chunk) {
    if (m_completed) return;
    if (chunk->level->dimension->id != m_dimension) return;

    if (m_schematic == nullptr)
        m_schematic = m_levelGenOptions->getSchematicFile(m_schematicName);

    if (!m_locationBox.has_value()) updateLocationBox();
    if (chunkBox->intersects(*m_locationBox)) {
        m_locationBox->y1 =
            std::min((double)Level::maxBuildHeight, m_locationBox->y1);

#ifdef _DEBUG
        app.DebugPrintf("Applying schematic %s to chunk (%d,%d)\n",
                        m_schematicName.c_str(), chunk->x, chunk->z);
#endif
        m_totalBlocksChanged += m_schematic->applyBlocksAndData(
            chunk, chunkBox, &*m_locationBox, m_rotation);

        // Add the tileEntities
        m_schematic->applyTileEntities(chunk, chunkBox, &*m_locationBox,
                                       m_rotation);

        // TODO This does not take into account things that go outside the
        // bounds of the world
        int targetBlocks = (m_locationBox->x1 - m_locationBox->x0) *
                           (m_locationBox->y1 - m_locationBox->y0) *
                           (m_locationBox->z1 - m_locationBox->z0);
        if ((m_totalBlocksChanged == targetBlocks) &&
            (m_totalBlocksChangedLighting == targetBlocks)) {
            m_completed = true;
            // m_levelGenOptions->releaseSchematicFile(m_schematicName);
            // m_schematic = nullptr;
        }
    }
}

void ApplySchematicRuleDefinition::processSchematicLighting(AABB* chunkBox,
                                                            LevelChunk* chunk) {
    if (m_completed) return;
    if (chunk->level->dimension->id != m_dimension) return;

    if (m_schematic == nullptr)
        m_schematic = m_levelGenOptions->getSchematicFile(m_schematicName);

    if (!m_locationBox.has_value()) updateLocationBox();
    if (chunkBox->intersects(*m_locationBox)) {
        m_locationBox->y1 =
            std::min((double)Level::maxBuildHeight, m_locationBox->y1);

#ifdef _DEBUG
        app.DebugPrintf("Applying schematic %s to chunk (%d,%d)\n",
                        m_schematicName.c_str(), chunk->x, chunk->z);
#endif
        m_totalBlocksChangedLighting += m_schematic->applyLighting(
            chunk, chunkBox, &*m_locationBox, m_rotation);

        // TODO This does not take into account things that go outside the
        // bounds of the world
        int targetBlocks = (m_locationBox->x1 - m_locationBox->x0) *
                           (m_locationBox->y1 - m_locationBox->y0) *
                           (m_locationBox->z1 - m_locationBox->z0);
        if ((m_totalBlocksChanged == targetBlocks) &&
            (m_totalBlocksChangedLighting == targetBlocks)) {
            m_completed = true;
            // m_levelGenOptions->releaseSchematicFile(m_schematicName);
            // m_schematic = nullptr;
        }
    }
}

bool ApplySchematicRuleDefinition::checkIntersects(int x0, int y0, int z0,
                                                   int x1, int y1, int z1) {
    if (!m_locationBox.has_value()) updateLocationBox();
    return m_locationBox->intersects(x0, y0, z0, x1, y1, z1);
}

int ApplySchematicRuleDefinition::getMinY() {
    if (!m_locationBox.has_value()) updateLocationBox();
    return m_locationBox->y0;
}

void ApplySchematicRuleDefinition::reset() {
    m_totalBlocksChanged = 0;
    m_totalBlocksChangedLighting = 0;
    m_completed = false;
}
