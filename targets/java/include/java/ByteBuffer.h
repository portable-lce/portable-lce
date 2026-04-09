#pragma once

#include <bit>
#include <cstdint>
#include <vector>

#include "Buffer.h"

class IntBuffer;
class FloatBuffer;

class ByteBuffer : public Buffer {
protected:
    uint8_t* buffer;
    std::endian byteOrder;

public:
    ByteBuffer(unsigned int capacity);
    static ByteBuffer* allocateDirect(int capacity);
    ByteBuffer(unsigned int capacity, uint8_t* backingArray);
    virtual ~ByteBuffer();

    static ByteBuffer* wrap(std::vector<uint8_t>& b);
    static ByteBuffer* allocate(unsigned int capacity);
    void order(std::endian a);
    ByteBuffer* flip();
    uint8_t* getBuffer();
    int getSize();
    int getInt();
    int getInt(unsigned int index);
    void get(std::vector<uint8_t>) {}  // 4J - TODO
    uint8_t get(int index);
    int64_t getLong();
    short getShort();
    void getShortArray(std::vector<short>& s);
    ByteBuffer* put(int index, uint8_t b);
    ByteBuffer* putInt(int value);
    ByteBuffer* putInt(unsigned int index, int value);
    ByteBuffer* putShort(short value);
    ByteBuffer* putShortArray(std::vector<short>& s);
    ByteBuffer* putLong(int64_t value);
    ByteBuffer* put(std::vector<uint8_t>& inputArray);
    std::vector<uint8_t> array();
    IntBuffer* asIntBuffer();
    FloatBuffer* asFloatBuffer();
};
