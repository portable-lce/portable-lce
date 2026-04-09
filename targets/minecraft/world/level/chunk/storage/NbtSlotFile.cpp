#include "NbtSlotFile.h"

#include <filesystem>

#include "java/File.h"

namespace {
std::FILE* OpenBinaryFileForReadWrite(const File& file) {
#if defined(_WIN32)
    std::FILE* stream = _wfopen(file.getPath().c_str(), "r+b");
    if (stream == nullptr) {
        stream = _wfopen(file.getPath().c_str(), "w+b");
    }
#else
    const std::string nativePath =
        std::filesystem::path(file.getPath()).string();
    std::FILE* stream = std::fopen(nativePath.c_str(), "r+b");
    if (stream == nullptr) {
        stream = std::fopen(nativePath.c_str(), "w+b");
    }
#endif
    return stream;
}

bool SeekFile(std::FILE* file, int64_t offset) {
#if defined(_WIN32)
    return _fseeki64(file, offset, SEEK_SET) == 0;
#else
    return fseeko(file, static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
}

bool ReadExact(std::FILE* file, void* buffer, std::size_t size) {
    return std::fread(buffer, 1, size, file) == size;
}

bool WriteExact(std::FILE* file, const void* buffer, std::size_t size) {
    return std::fwrite(buffer, 1, size, file) == size;
}
}  // namespace

std::vector<uint8_t> NbtSlotFile::READ_BUFFER(1024 * 1024);
int64_t NbtSlotFile::largest = 0;

NbtSlotFile::NbtSlotFile(File file) {
    totalFileSlots = 0;
    fileSlotMapLength =
        ZonedChunkStorage::CHUNKS_PER_ZONE * ZonedChunkStorage::CHUNKS_PER_ZONE;
    fileSlotMap = new std::vector<int>*[fileSlotMapLength];

    if (!file.exists() || file.length()) {
        raf = OpenBinaryFileForReadWrite(file);
        writeHeader();
    } else {
        raf = OpenBinaryFileForReadWrite(file);
    }

    readHeader();

    for (int i = 0; i < fileSlotMapLength; i++) {
        fileSlotMap[i] = new std::vector<int>;
    }

    for (int fileSlot = 0; fileSlot < totalFileSlots; fileSlot++) {
        seekSlotHeader(fileSlot);
        short slot;
        ReadExact(raf, &slot, sizeof(slot));
        if (slot == 0) {
            freeFileSlots.push_back(fileSlot);
        } else if (slot < 0) {
            fileSlotMap[(-slot) - 1]->push_back(fileSlot);
        } else {
            fileSlotMap[slot - 1]->push_back(fileSlot);
        }
    }
}

void NbtSlotFile::readHeader() {
    SeekFile(raf, 0);
    int magic;
    ReadExact(raf, &magic, sizeof(magic));
    //    if (magic != MAGIC_NUMBER) throw new IOException("Bad magic number: "
    //    + magic);		// 4J - TODO
    short version;
    ReadExact(raf, &version, sizeof(version));
    //    if (version != 0) throw new IOException("Bad version number: " +
    //    version);		// 4J - TODO
    ReadExact(raf, &totalFileSlots, sizeof(totalFileSlots));
}

void NbtSlotFile::writeHeader() {
    short version = 0;
    SeekFile(raf, 0);
    WriteExact(raf, &MAGIC_NUMBER, sizeof(MAGIC_NUMBER));
    WriteExact(raf, &version, sizeof(version));
    WriteExact(raf, &totalFileSlots, sizeof(totalFileSlots));
}

void NbtSlotFile::seekSlotHeader(int fileSlot) {
    int target =
        FILE_HEADER_SIZE + fileSlot * (FILE_SLOT_SIZE + FILE_SLOT_HEADER_SIZE);
    SeekFile(raf, target);
}

void NbtSlotFile::seekSlot(int fileSlot) {
    int target =
        FILE_HEADER_SIZE + fileSlot * (FILE_SLOT_SIZE + FILE_SLOT_HEADER_SIZE);
    SeekFile(raf, target + FILE_SLOT_HEADER_SIZE);
}

std::vector<CompoundTag*>* NbtSlotFile::readAll(int slot) {
    std::vector<CompoundTag*>* tags = new std::vector<CompoundTag*>;
    std::vector<int>* fileSlots = fileSlotMap[slot];
    int skipped = 0;

    auto itEnd = fileSlots->end();
    for (auto it = fileSlots->begin(); it != itEnd; it++) {
        int c = *it;  // fileSlots->at(i);

        int pos = 0;
        int continuesAt = -1;
        int expectedSlot = slot + 1;
        do {
            seekSlotHeader(c);
            short oldSlot;
            ReadExact(raf, &oldSlot, sizeof(oldSlot));
            short size;
            ReadExact(raf, &size, sizeof(size));
            ReadExact(raf, &continuesAt, sizeof(continuesAt));
            int lastSlot;
            ReadExact(raf, &lastSlot, sizeof(lastSlot));

            seekSlot(c);
            if (expectedSlot > 0 && oldSlot == -expectedSlot) {
                skipped++;
                goto fileSlotLoop;  // 4J - used to be continue fileSlotLoop,
                                    // with for loop labelled as fileSlotLoop
            }

            //            if (oldSlot != expectedSlot) throw new
            //            IOException("Wrong slot! Got " + oldSlot + ", expected
            //            " + expectedSlot);	// 4J - TODO

            ReadExact(raf, READ_BUFFER.data() + pos, size);

            if (continuesAt >= 0) {
                pos += size;
                c = continuesAt;
                expectedSlot = -slot - 1;
            }
        } while (continuesAt >= 0);
        tags->push_back(NbtIo::decompress(READ_BUFFER));
    fileSlotLoop:
        continue;
    }

    return tags;
}

int NbtSlotFile::getFreeSlot() {
    int fileSlot;

    // 4J - removed - don't see how toReplace can ever have anything in here,
    // and might not be initialised
    //    if (toReplace->size() > 0)
    //	{
    //		fileSlot = toReplace->back();
    //		toReplace->pop_back();
    //    } else

    if (freeFileSlots.size() > 0) {
        fileSlot = freeFileSlots.back();
        freeFileSlots.pop_back();
    } else {
        fileSlot = totalFileSlots++;
        writeHeader();
    }

    return fileSlot;
}
void NbtSlotFile::replaceSlot(int slot, std::vector<CompoundTag*>* tags) {
    toReplace = fileSlotMap[slot];
    fileSlotMap[slot] = new std::vector<int>();

    auto itEndTags = tags->end();
    for (auto it = tags->begin(); it != itEndTags; it++) {
        CompoundTag* tag = *it;  // tags->at(i);
        std::vector<uint8_t> compressed = NbtIo::compress(tag);
        if (compressed.size() > largest) {
            char buf[256];
            largest = compressed.size();
#ifndef _CONTENT_PACKAGE
            snprintf(buf, 256, "New largest: %I64d (%s)\n", largest,
                     tag->getString("id").c_str());
            OutputDebugStringW(buf);
#endif
        }

        int pos = 0;
        int remaining = compressed.size();
        if (remaining == 0) continue;

        int nextFileSlot = getFreeSlot();
        short currentSlot = slot + 1;
        int lastFileSlot = -1;

        while (remaining > 0) {
            int fileSlot = nextFileSlot;
            fileSlotMap[slot]->push_back(fileSlot);

            short toWrite = remaining;
            if (toWrite > FILE_SLOT_SIZE) {
                toWrite = FILE_SLOT_SIZE;
            }

            remaining -= toWrite;
            if (remaining > 0) {
                nextFileSlot = getFreeSlot();
            } else {
                nextFileSlot = -1;
            }

            seekSlotHeader(fileSlot);
            WriteExact(raf, &currentSlot, sizeof(currentSlot));
            WriteExact(raf, &toWrite, sizeof(toWrite));
            WriteExact(raf, &nextFileSlot, sizeof(nextFileSlot));
            WriteExact(raf, &lastFileSlot, sizeof(lastFileSlot));

            seekSlot(fileSlot);
            WriteExact(raf, compressed.data() + pos, toWrite);

            if (remaining > 0) {
                lastFileSlot = fileSlot;
                pos += toWrite;
                currentSlot = -slot - 1;
            }
        }
    }

    auto itEndToRep = toReplace->end();
    for (auto it = toReplace->begin(); it != itEndToRep; it++) {
        int c = *it;  // toReplace->at(i);

        freeFileSlots.push_back(c);

        seekSlotHeader(c);
        short zero = 0;
        WriteExact(raf, &zero, sizeof(zero));
    }

    toReplace->clear();
}

void NbtSlotFile::close() {
    if (raf != nullptr) {
        std::fclose(raf);
        raf = nullptr;
    }
}
