#pragma once
#include "Tag.h"

class IntTag : public Tag {
public:
    int data;
    IntTag(const std::string& name) : Tag(name) {}
    IntTag(const std::string& name, int data) : Tag(name) { this->data = data; }

    void write(DataOutput* dos) { dos->writeInt(data); }
    void load(DataInput* dis, int tagDepth) { data = dis->readInt(); }

    uint8_t getId() { return TAG_Int; }
    std::string toString() {
        static char buf[32];
        snprintf(buf, 32, "%d", data);
        return std::string(buf);
    }

    Tag* copy() { return new IntTag(getName(), data); }

    bool equals(Tag* obj) {
        if (Tag::equals(obj)) {
            IntTag* o = (IntTag*)obj;
            return data == o->data;
        }
        return false;
    }
};