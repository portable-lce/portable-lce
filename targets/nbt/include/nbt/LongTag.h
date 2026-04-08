#pragma once
#include "Tag.h"

class LongTag : public Tag {
public:
    int64_t data;
    LongTag(const std::string& name) : Tag(name) {}
    LongTag(const std::string& name, int64_t data) : Tag(name) {
        this->data = data;
    }

    void write(DataOutput* dos) { dos->writeLong(data); }
    void load(DataInput* dis, int tagDepth) { data = dis->readLong(); }

    uint8_t getId() { return TAG_Long; }
    std::string toString() {
        static char buf[32];
        snprintf(buf, 32, "%lld", static_cast<long long>(data));
        return std::string(buf);
    }

    Tag* copy() { return new LongTag(getName(), data); }

    bool equals(Tag* obj) {
        if (Tag::equals(obj)) {
            LongTag* o = (LongTag*)obj;
            return data == o->data;
        }
        return false;
    }
};