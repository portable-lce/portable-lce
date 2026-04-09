#pragma once
// 4J Stu - We are not using GZIP compression, so this is just a pass through
// class

#include "OutputStream.h"

class GZIPOutputStream : public OutputStream {
private:
    OutputStream* stream;

public:
    GZIPOutputStream(OutputStream* out) : stream(out){};
    virtual void write(unsigned int b) { stream->write(b); };
    virtual void write(const std::vector<uint8_t>& b) { stream->write(b); };
    virtual void write(const std::vector<uint8_t>& b, unsigned int offset,
                       unsigned int length) {
        stream->write(b, offset, length);
    };
    virtual void close() { stream->close(); };
    virtual void flush() {}
};