#include "ArchiveFile.h"

#include <assert.h>

#include <cstdlib>
#include <utility>

#include "app/linux/LinuxGame.h"
#include "platform/fs/fs.h"
#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"
#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "java/InputOutputStream/FileInputStream.h"

void ArchiveFile::_readHeader(DataInputStream* dis) {
    int numberOfFiles = dis->readInt();

    for (int i = 0; i < numberOfFiles; i++) {
        MetaData* meta = new MetaData();
        meta->filename = dis->readUTF();
        meta->ptr = dis->readInt();
        meta->filesize = dis->readInt();

        // Filenames preceeded by an asterisk have been compressed.
        if (meta->filename[0] == '*') {
            meta->filename = meta->filename.substr(1);
            meta->isCompressed = true;
        } else
            meta->isCompressed = false;

        m_index.insert(
            std::pair<std::string, PMetaData>(meta->filename, meta));
    }
}

ArchiveFile::ArchiveFile(File file) {
    m_cachedData = nullptr;
    m_sourcefile = file;
    app.DebugPrintf("Loading archive file...\n");
#if !defined(_CONTENT_PACKAGE)
    app.DebugPrintf("archive file - %s\n", file.getPath().c_str());
#endif

    if (!file.exists()) {
        app.DebugPrintf("Failed to load archive file!\n");  //,file.getPath());
        app.FatalLoadError();
    }

    FileInputStream fis(file);

#if defined(_WINDOWS64)
    std::vector<uint8_t> readArray(file.length());
    fis.read(readArray, 0, file.length());

    ByteArrayInputStream bais(readArray);
    DataInputStream dis(&bais);

    m_cachedData = readArray.data();
#else
    DataInputStream dis(&fis);
#endif

    _readHeader(&dis);

    dis.close();
    fis.close();
#if defined(_WINDOWS64)
    bais.reset();
#endif
    app.DebugPrintf("Finished loading archive file\n");
}

ArchiveFile::~ArchiveFile() { delete m_cachedData; }

std::vector<std::string>* ArchiveFile::getFileList() {
    std::vector<std::string>* out = new std::vector<std::string>();

    for (auto it = m_index.begin(); it != m_index.end(); it++)

        out->push_back(it->first);

    return out;
}

bool ArchiveFile::hasFile(const std::string& filename) {
    return m_index.find(filename) != m_index.end();
}

int ArchiveFile::getFileSize(const std::string& filename) {
    return hasFile(filename) ? m_index.at(filename)->filesize : -1;
}

std::vector<uint8_t> ArchiveFile::getFile(const std::string& filename) {
    std::vector<uint8_t> out;
    auto it = m_index.find(filename);

    if (it == m_index.end()) {
        app.DebugPrintf("Couldn't find file in archive\n");
        app.DebugPrintf("Failed to find file '%s' in archive\n",
                        filename.c_str());
#if !defined(_CONTENT_PACKAGE)
        assert(0);
#endif
        app.FatalLoadError();
    } else {
        PMetaData data = it->second;

#if defined(_WINDOWS64)
        out = std::vector<uint8_t>(data->filesize);

        memcpy(out.data(), m_cachedData + data->ptr, data->filesize);
#else
        const unsigned int fileSize = static_cast<unsigned int>(data->filesize);
        std::uint8_t* pbData = new std::uint8_t[fileSize == 0 ? 1 : fileSize];
        out = std::vector<uint8_t>(pbData, pbData + fileSize);
        auto readResult =
            PlatformFilesystem.readFileSegment(
                m_sourcefile.getPath(), static_cast<std::size_t>(data->ptr),
                out.data(), static_cast<std::size_t>(data->filesize));

        if (readResult.status != IPlatformFilesystem::ReadStatus::Ok) {
            app.DebugPrintf("Failed to read archive file segment\n");
            app.FatalLoadError();
        }
#endif

        // Compressed filenames are preceeded with an asterisk.
        if (data->isCompressed && !out.empty()) {
            /* 4J-JEV:
             * If a compressed file is accessed before compression object is
             * initialized it will crash here (Compression::getCompression).
             */
            /// 4 279 553 556

            ByteArrayInputStream bais(out);
            DataInputStream dis(&bais);
            unsigned int decompressedSize = dis.readInt();
            dis.close();

            std::uint8_t* uncompressedBuffer =
                new std::uint8_t[decompressedSize];
            Compression::getCompression()->Decompress(
                uncompressedBuffer, &decompressedSize, out.data() + 4,
                out.size() - 4);

            out = std::vector<uint8_t>(uncompressedBuffer,
                                       uncompressedBuffer + decompressedSize);
            delete[] uncompressedBuffer;
        }

        assert(!out.empty());  // THERE IS NO FILE WITH THIS NAME!
    }

    return out;
}
