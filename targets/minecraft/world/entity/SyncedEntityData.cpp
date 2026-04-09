#include "SyncedEntityData.h"

#include <assert.h>

#include <vector>

#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/DataOutputStream.h"
#include "minecraft/network/packet/Packet.h"
#include "minecraft/util/Log.h"

class ItemInstance;

SynchedEntityData::SynchedEntityData() {
    m_isDirty = false;
    m_isEmpty = true;
}

void SynchedEntityData::define(int id, int value) {
    checkId(id);
    int type = TYPE_INT;
    std::shared_ptr<DataItem> dataItem =
        std::make_shared<DataItem>(type, id, value);
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, uint8_t value) {
    checkId(id);
    int type = TYPE_BYTE;
    std::shared_ptr<DataItem> dataItem =
        std::make_shared<DataItem>(type, id, value);
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, short value) {
    checkId(id);
    int type = TYPE_SHORT;
    std::shared_ptr<DataItem> dataItem =
        std::make_shared<DataItem>(type, id, value);
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, float value) {
    checkId(id);
    int type = TYPE_FLOAT;
    std::shared_ptr<DataItem> dataItem =
        std::make_shared<DataItem>(type, id, value);
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::define(int id, const std::string& value) {
    checkId(id);
    int type = TYPE_STRING;
    std::shared_ptr<DataItem> dataItem =
        std::make_shared<DataItem>(type, id, value);
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::defineNULL(int id, void* pVal) {
    checkId(id);
    int type = TYPE_ITEMINSTANCE;
    std::shared_ptr<DataItem> dataItem = std::shared_ptr<DataItem>(
        new DataItem(type, id, std::shared_ptr<ItemInstance>()));
    itemsById[id] = dataItem;
    m_isEmpty = false;
}

void SynchedEntityData::checkId(int id) {}

uint8_t SynchedEntityData::getByte(int id) {
    return itemsById[id]->getValue_byte();
}

short SynchedEntityData::getShort(int id) {
    return itemsById[id]->getValue_short();
}

int SynchedEntityData::getInteger(int id) {
    return itemsById[id]->getValue_int();
}

float SynchedEntityData::getFloat(int id) {
    return itemsById[id]->getValue_float();
}

std::string SynchedEntityData::getString(int id) {
    return itemsById[id]->getValue_wstring();
}

std::shared_ptr<ItemInstance> SynchedEntityData::getItemInstance(int id) {
    // assert(false);	// 4J - not currently implemented
    return itemsById[id]->getValue_itemInstance();
}

Pos* SynchedEntityData::getPos(int id) {
    assert(false);  // 4J - not currently implemented
    return nullptr;
}

void SynchedEntityData::set(int id, int value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_int()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::set(int id, uint8_t value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_byte()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::set(int id, short value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_short()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::set(int id, float value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_float()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::set(int id, const std::string& value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_wstring()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::set(int id, std::shared_ptr<ItemInstance> value) {
    std::shared_ptr<DataItem> dataItem = itemsById[id];

    // update the value if it has changed
    if (value != dataItem->getValue_itemInstance()) {
        dataItem->setValue(value);
        dataItem->setDirty(true);
        m_isDirty = true;
    }
}

void SynchedEntityData::markDirty(int id) {
    itemsById[id]->dirty = true;
    m_isDirty = true;
}

bool SynchedEntityData::isDirty() { return m_isDirty; }

void SynchedEntityData::pack(
    std::vector<std::shared_ptr<DataItem> >* items,
    DataOutputStream* output)  // TODO throws IOException
{
    if (items != nullptr) {
        auto itEnd = items->end();
        for (auto it = items->begin(); it != itEnd; it++) {
            std::shared_ptr<DataItem> dataItem = *it;
            writeDataItem(output, dataItem);
        }
    }

    // add an eof
    output->writeByte(EOF_MARKER);
}

std::vector<std::shared_ptr<SynchedEntityData::DataItem> >*
SynchedEntityData::packDirty() {
    std::vector<std::shared_ptr<DataItem> >* result = nullptr;

    if (m_isDirty) {
        for (int i = 0; i <= MAX_ID_VALUE; i++) {
            std::shared_ptr<DataItem> dataItem = itemsById[i];
            if ((dataItem != nullptr) && dataItem->isDirty()) {
                dataItem->setDirty(false);

                if (result == nullptr) {
                    result = new std::vector<std::shared_ptr<DataItem> >();
                }
                result->push_back(dataItem);
            }
        }
    }
    m_isDirty = false;

    return result;
}

void SynchedEntityData::packAll(DataOutputStream* output)  // throws IOException
{
    for (int i = 0; i <= MAX_ID_VALUE; i++) {
        std::shared_ptr<DataItem> dataItem = itemsById[i];
        if (dataItem != nullptr) {
            writeDataItem(output, dataItem);
        }
    }

    // add an eof
    output->writeByte(EOF_MARKER);
}

std::vector<std::shared_ptr<SynchedEntityData::DataItem> >*
SynchedEntityData::getAll() {
    std::vector<std::shared_ptr<DataItem> >* result = nullptr;

    for (int i = 0; i <= MAX_ID_VALUE; i++) {
        std::shared_ptr<DataItem> dataItem = itemsById[i];
        if (dataItem != nullptr) {
            if (result == nullptr) {
                result = new std::vector<std::shared_ptr<DataItem> >();
            }
            result->push_back(dataItem);
        }
    }

    return result;
}

void SynchedEntityData::writeDataItem(
    DataOutputStream* output,
    std::shared_ptr<DataItem> dataItem)  // throws IOException
{
    // pack type and id
    int header = ((dataItem->getType() << TYPE_SHIFT) |
                  (dataItem->getId() & MAX_ID_VALUE)) &
                 0xff;
    output->writeByte(header);

    // write value
    switch (dataItem->getType()) {
        case TYPE_BYTE:
            output->writeByte(dataItem->getValue_byte());
            break;
        case TYPE_INT:
            output->writeInt(dataItem->getValue_int());
            break;
        case TYPE_SHORT:
            output->writeShort(dataItem->getValue_short());
            break;
        case TYPE_FLOAT:
            output->writeFloat(dataItem->getValue_float());
            break;
        case TYPE_STRING:
            Packet::writeUtf(dataItem->getValue_wstring(), output);
            break;
        case TYPE_ITEMINSTANCE: {
            std::shared_ptr<ItemInstance> instance =
                (std::shared_ptr<ItemInstance>)
                    dataItem->getValue_itemInstance();
            Packet::writeItem(instance, output);
        } break;

        default:
            assert(false);  // 4J - not implemented
            break;
    }
}

std::vector<std::shared_ptr<SynchedEntityData::DataItem> >*
SynchedEntityData::unpack(DataInputStream* input)  // throws IOException
{
    std::vector<std::shared_ptr<DataItem> >* result = nullptr;

    int currentHeader = input->readByte();

    while (currentHeader != EOF_MARKER) {
        if (result == nullptr) {
            result = new std::vector<std::shared_ptr<DataItem> >();
        }

        // split type and id
        int itemType = (currentHeader & TYPE_MASK) >> TYPE_SHIFT;
        int itemId = (currentHeader & MAX_ID_VALUE);

        std::shared_ptr<DataItem> item = std::shared_ptr<DataItem>();
        switch (itemType) {
            case TYPE_BYTE: {
                uint8_t dataRead = input->readByte();
                item = std::shared_ptr<DataItem>(
                    new DataItem(itemType, itemId, dataRead));
            } break;
            case TYPE_SHORT: {
                short dataRead = input->readShort();
                item = std::shared_ptr<DataItem>(
                    new DataItem(itemType, itemId, dataRead));
            } break;
            case TYPE_INT: {
                int dataRead = input->readInt();
                item = std::shared_ptr<DataItem>(
                    new DataItem(itemType, itemId, dataRead));
            } break;
            case TYPE_FLOAT: {
                float dataRead = input->readFloat();
                item = std::shared_ptr<DataItem>(
                    new DataItem(itemType, itemId, dataRead));

            } break;
            case TYPE_STRING:
                item = std::make_shared<DataItem>(
                    itemType, itemId,
                    Packet::readUtf(input, MAX_STRING_DATA_LENGTH));
                break;
            case TYPE_ITEMINSTANCE: {
                item = std::shared_ptr<DataItem>(
                    new DataItem(itemType, itemId, Packet::readItem(input)));
            } break;
            default:
                Log::info(
                    " ------ garbage data, or early end of stream due to an "
                    "incomplete packet\n");
                delete result;
                return nullptr;
                break;
        }
        result->push_back(item);

        currentHeader = input->readByte();
    }

    return result;
}

/**
 * Assigns values from a list of data items.
 *
 * @param items
 */

void SynchedEntityData::assignValues(
    std::vector<std::shared_ptr<DataItem> >* items) {
    auto itEnd = items->end();
    for (auto it = items->begin(); it != itEnd; it++) {
        std::shared_ptr<DataItem> item = *it;

        std::shared_ptr<DataItem> itemFromId = itemsById[item->getId()];
        if (itemFromId != nullptr) {
            switch (item->getType()) {
                case TYPE_BYTE:
                    itemFromId->setValue(item->getValue_byte());
                    break;
                case TYPE_SHORT:
                    itemFromId->setValue(item->getValue_short());
                    break;
                case TYPE_INT:
                    itemFromId->setValue(item->getValue_int());
                    break;
                case TYPE_FLOAT:
                    itemFromId->setValue(item->getValue_float());
                    break;
                case TYPE_STRING:
                    itemFromId->setValue(item->getValue_wstring());
                    break;
                case TYPE_ITEMINSTANCE:
                    itemFromId->setValue(item->getValue_itemInstance());
                    break;
                default:
                    assert(false);  // 4J - not implemented
                    break;
            }
        }
    }

    // client-side dirty
    m_isDirty = true;
}

bool SynchedEntityData::isEmpty() { return m_isEmpty; }

void SynchedEntityData::clearDirty() { m_isDirty = false; }

int SynchedEntityData::getSizeInBytes() {
    int size = 1;

    for (int i = 0; i <= MAX_ID_VALUE; i++) {
        std::shared_ptr<DataItem> dataItem = itemsById[i];
        if (dataItem != nullptr) {
            size += 1;

            // write value
            switch (dataItem->getType()) {
                case TYPE_BYTE:
                    size += 1;
                    break;
                case TYPE_SHORT:
                    size += 2;
                    break;
                case TYPE_INT:
                    size += 4;
                    break;
                case TYPE_FLOAT:
                    size += 4;
                    break;
                case TYPE_STRING:
                    size += (int)dataItem->getValue_wstring().length() +
                            2;  // Estimate, assuming all ascii chars
                    break;
                case TYPE_ITEMINSTANCE:
                    // short + byte + short
                    size += 2 + 1 + 2;  // Estimate, assuming all ascii chars
                    break;
                default:
                    break;
            }
        }
    }
    return size;
}

//////////////////
// DataItem class
/////////////////

SynchedEntityData::DataItem::DataItem(int type, int id, int value)
    : type(type), id(id) {
    this->value_int = value;
    this->dirty = true;
}

SynchedEntityData::DataItem::DataItem(int type, int id, uint8_t value)
    : type(type), id(id) {
    this->value_byte = value;
    this->dirty = true;
}

SynchedEntityData::DataItem::DataItem(int type, int id, short value)
    : type(type), id(id) {
    this->value_short = value;
    this->dirty = true;
}

SynchedEntityData::DataItem::DataItem(int type, int id, float value)
    : type(type), id(id) {
    this->value_float = value;
    this->dirty = true;
}

SynchedEntityData::DataItem::DataItem(int type, int id,
                                      const std::string& value)
    : type(type), id(id) {
    this->value_wstring = value;
    this->dirty = true;
}

SynchedEntityData::DataItem::DataItem(
    int type, int id, std::shared_ptr<ItemInstance> itemInstance)
    : type(type), id(id) {
    this->value_itemInstance = itemInstance;
    this->dirty = true;
}

int SynchedEntityData::DataItem::getId() { return id; }

void SynchedEntityData::DataItem::setValue(int value) {
    this->value_int = value;
}

void SynchedEntityData::DataItem::setValue(uint8_t value) {
    this->value_byte = value;
}

void SynchedEntityData::DataItem::setValue(short value) {
    this->value_short = value;
}

void SynchedEntityData::DataItem::setValue(float value) {
    this->value_float = value;
}

void SynchedEntityData::DataItem::setValue(const std::string& value) {
    this->value_wstring = value;
}

void SynchedEntityData::DataItem::setValue(
    std::shared_ptr<ItemInstance> itemInstance) {
    this->value_itemInstance = itemInstance;
}

int SynchedEntityData::DataItem::getValue_int() { return value_int; }

short SynchedEntityData::DataItem::getValue_short() { return value_short; }

float SynchedEntityData::DataItem::getValue_float() { return value_float; }

uint8_t SynchedEntityData::DataItem::getValue_byte() { return value_byte; }

std::string SynchedEntityData::DataItem::getValue_wstring() {
    return value_wstring;
}

std::shared_ptr<ItemInstance>
SynchedEntityData::DataItem::getValue_itemInstance() {
    return value_itemInstance;
}

int SynchedEntityData::DataItem::getType() { return type; }

bool SynchedEntityData::DataItem::isDirty() { return dirty; }

void SynchedEntityData::DataItem::setDirty(bool dirty) { this->dirty = dirty; }
