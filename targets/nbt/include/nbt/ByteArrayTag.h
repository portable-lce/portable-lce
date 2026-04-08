#pragma once
#include "Tag.h"
#include "java/System.h"

class ByteArrayTag : public Tag {
public:
    std::vector<uint8_t> data;
    bool m_ownData;

    ByteArrayTag(const std::string& name) : Tag(name) { m_ownData = false; }
    ByteArrayTag(const std::string& name, std::vector<uint8_t>& data,
                 bool ownData = false)
        : Tag(name) {
        this->data = data;
        m_ownData = ownData;
    }  // 4J - added ownData param
    ~ByteArrayTag() {}

    void write(DataOutput* dos) {
        dos->writeInt(data.size());
        dos->write(data);
    }

    void load(DataInput* dis, int tagDepth) {
        int length = dis->readInt();

        data = std::vector<uint8_t>(length);
        dis->readFully(data);
    }

    uint8_t getId() { return TAG_Byte_Array; }

    std::string toString() {
        static char buf[32];
        snprintf(buf, 32, "[%zu bytes]", data.size());
        return std::string(buf);
    }

    bool equals(Tag* obj) {
        if (Tag::equals(obj)) {
            ByteArrayTag* o = (ByteArrayTag*)obj;
            return ((data.empty() && o->data.empty()) ||
                    (!data.empty() && data.size() == o->data.size() &&
                     memcmp(data.data(), o->data.data(), data.size()) == 0));
        }
        return false;
    }

    Tag* copy() {
        std::vector<uint8_t> cp(data.size());
        std::copy(data.begin(), data.end(), cp.begin());
        return new ByteArrayTag(getName(), cp, true);
    }
};