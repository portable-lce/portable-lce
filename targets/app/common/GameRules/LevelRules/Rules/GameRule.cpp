
#include "minecraft/world/level/GameRules/GameRule.h"

#include <wchar.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "minecraft/world/level/GameRules/GameRuleDefinition.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"

class Connection;
class ItemInstance;

GameRule::GameRule(GameRuleDefinition* definition, Connection* connection) {
    m_definition = definition;
    m_connection = connection;
}

GameRule::~GameRule() {
    for (auto it = m_parameters.begin(); it != m_parameters.end(); ++it) {
        if (it->second.isPointer) {
            delete it->second.gr;
        }
    }
}

GameRule::ValueType GameRule::getParameter(const std::string& parameterName) {
    if (m_parameters.find(parameterName) == m_parameters.end()) {
#ifndef _CONTENT_PACKAGE
        printf("WARNING: Parameter %s was not set before being fetched\n",
                parameterName.c_str());
        assert(0);
#endif
    }
    return m_parameters[parameterName];
}

void GameRule::setParameter(const std::string& parameterName,
                            ValueType value) {
    if (m_parameters.find(parameterName) == m_parameters.end()) {
#ifndef _CONTENT_PACKAGE
        printf("Adding parameter %s to GameRule\n", parameterName.c_str());
#endif
    } else {
#ifndef _CONTENT_PACKAGE
        printf("Setting parameter %s for GameRule\n", parameterName.c_str());
#endif
    }
    m_parameters[parameterName] = value;
}

GameRuleDefinition* GameRule::getGameRuleDefinition() { return m_definition; }

void GameRule::onUseTile(int tileId, int x, int y, int z) {
    m_definition->onUseTile(this, tileId, x, y, z);
}
void GameRule::onCollectItem(std::shared_ptr<ItemInstance> item) {
    m_definition->onCollectItem(this, item);
}

void GameRule::write(DataOutputStream* dos) {
    // Find required parameters.
    dos->writeInt(m_parameters.size());
    for (auto it = m_parameters.begin(); it != m_parameters.end(); it++) {
        std::string pName = (*it).first;
        ValueType vType = (*it).second;

        dos->writeUTF((*it).first);
        dos->writeBoolean(vType.isPointer);

        if (vType.isPointer)
            vType.gr->write(dos);
        else
            dos->writeLong(vType.i64);
    }
}

void GameRule::read(DataInputStream* dis) {
    int savedParams = dis->readInt();
    for (int i = 0; i < savedParams; i++) {
        std::string pNames = dis->readUTF();

        ValueType vType = getParameter(pNames);

        if (dis->readBoolean()) {
            vType.gr->read(dis);
        } else {
            vType.isPointer = false;
            vType.i64 = dis->readLong();
            setParameter(pNames, vType);
        }
    }
}
