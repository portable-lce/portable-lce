#include "ZoneFile.h"

#include <filesystem>

#include "java/ByteBuffer.h"
#include "java/File.h"

namespace {
std::FILE* OpenBinaryFileForReadWrite(const File& file) {
    const std::string nativePath =
        std::filesystem::path(file.getPath()).string();
    std::FILE* stream = std::fopen(nativePath.c_str(), "r+b");
    if (stream == nullptr) {
        stream = std::fopen(nativePath.c_str(), "w+b");
    }
    return stream;
}
}  // namespace

const int ZoneFile::slotsLength =
    ZonedChunkStorage::CHUNKS_PER_ZONE * ZonedChunkStorage::CHUNKS_PER_ZONE;

ZoneFile::ZoneFile(int64_t key, File file, File entityFile)
    : slots(slotsLength) {
    lastUse = 0;

    this->key = key;
    this->file = file;

    // 4J - try/catch removed
    //    try {
    this->entityFile = new NbtSlotFile(entityFile);
    //    } catch (Exception e) {
    //        System.out.println("Broken entity file: " + entityFile + " (" +
    //        e.toString() + "), replacing.."); entityFile.delete();
    //        entityFile.createNewFile();
    //        this.entityFile = new NbtSlotFile(entityFile);
    //    }

    channel = OpenBinaryFileForReadWrite(file);
    // 4J - try/catch removed
    //    try {
    readHeader();
    //    } catch (Exception e) {
    //        e.printStackTrace();
    //        throw new IOException("Broken zone file: " + file + ": " + e);
    //    }
}

void ZoneFile::readHeader() {
    ZoneIo* zoneIo = new ZoneIo(channel, 0);
    ByteBuffer* bb = zoneIo->read(FILE_HEADER_SIZE);
    bb->flip();
    if (bb->remaining() < 5) return;
    int magic = bb->getInt();
    //    if (magic != MAGIC_NUMBER) throw new IOException("Bad magic number: "
    //    + magic);		// 4J - TODO
    short version = bb->getShort();
    //    if (version != 0) throw new IOException("Bad version number: " +
    //    version);	// 4J - TODO

    slotCount = bb->getShort();
    bb->getShortArray(slots);
    bb->position(bb->position() + slotsLength * 2);
}

void ZoneFile::writeHeader() {
    ZoneIo* zoneIo = new ZoneIo(channel, 0);

    ByteBuffer* bb = ByteBuffer::allocate(FILE_HEADER_SIZE);
    bb->order(ZonedChunkStorage::BYTEORDER);
    bb->putInt(MAGIC_NUMBER);
    bb->putShort((short)0);
    bb->putShort((short)slotCount);
    bb->putShortArray(slots);
    bb->position(bb->position() + slots.size() * 2);
    bb->flip();
    zoneIo->write(bb, FILE_HEADER_SIZE);
}

void ZoneFile::close() {
    if (channel != nullptr) {
        std::fclose(channel);
        channel = nullptr;
    }
    entityFile->close();
}

ZoneIo* ZoneFile::getZoneIo(int slot) {
    if (slots[slot] == 0) {
        slots[slot] = ++slotCount;
        writeHeader();
    }
    int byteOffs = (slots[slot] - 1) * ZonedChunkStorage::CHUNK_SIZE_BYTES +
                   FILE_HEADER_SIZE;
    return new ZoneIo(channel, byteOffs);
}

bool ZoneFile::containsSlot(int slot) { return slots[slot] > 0; }
