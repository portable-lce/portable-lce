#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "minecraft/world/level/ConsoleGameRulesConstants.h"
#include "Packet.h"
#include "minecraft/network/packet/Packet.h"

class UpdateGameRuleProgressPacket
    : public Packet,
      public std::enable_shared_from_this<UpdateGameRuleProgressPacket> {
public:
    ConsoleGameRules::EGameRuleType m_definitionType;
    std::string m_messageId;
    int m_icon, m_auxValue;
    int m_dataTag;
    std::vector<uint8_t> m_data;

    UpdateGameRuleProgressPacket();
    UpdateGameRuleProgressPacket(ConsoleGameRules::EGameRuleType definitionType,
                                 const std::string& messageId, int icon,
                                 int auxValue, int dataTag, void* data,
                                 int dataLength);

    virtual void read(DataInputStream* dis);
    virtual void write(DataOutputStream* dos);
    virtual void handle(PacketListener* listener);
    virtual int getEstimatedSize();

public:
    static std::shared_ptr<Packet> create() {
        return std::make_shared<UpdateGameRuleProgressPacket>();
    }
    virtual int getId() { return 158; }
};