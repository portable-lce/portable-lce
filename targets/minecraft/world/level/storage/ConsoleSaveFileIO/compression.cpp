#include "minecraft/world/level/storage/ConsoleSaveFileIO/compression.h"

#include <assert.h>
#include <string.h>
#include <zlib.h>

#include <cstdint>
#include <mutex>

thread_local Compression::ThreadStorage* Compression::m_tlsCompression =
    nullptr;
Compression::ThreadStorage* Compression::m_tlsCompressionDefault = nullptr;

Compression::ThreadStorage::ThreadStorage() { compression = new Compression(); }
Compression::ThreadStorage::~ThreadStorage() { delete compression; }

void Compression::CreateNewThreadStorage() {
    ThreadStorage* tls = new ThreadStorage();
    if (m_tlsCompressionDefault == nullptr) {
        m_tlsCompressionDefault = tls;
    }
    m_tlsCompression = tls;
}

void Compression::UseDefaultThreadStorage() {
    m_tlsCompression = m_tlsCompressionDefault;
}

void Compression::ReleaseThreadStorage() {
    if (m_tlsCompression != m_tlsCompressionDefault) {
        delete m_tlsCompression;
    }
}

Compression* Compression::getCompression() {
    return m_tlsCompression->compression;
}

int32_t Compression::CompressLZXRLE(void* pDestination, unsigned int* pDestSize,
                                    void* pSource, unsigned int SrcSize) {
    std::lock_guard<std::mutex> lock(rleCompressLock);

    if (rleCompressBuf.size() < SrcSize * 2) rleCompressBuf.resize(SrcSize * 2);

    unsigned char* pucIn = (unsigned char*)pSource;
    unsigned char* pucEnd = pucIn + SrcSize;
    unsigned char* pucOut = rleCompressBuf.data();

    do {
        unsigned char thisOne = *pucIn++;
        unsigned int count = 1;
        while ((pucIn != pucEnd) && (*pucIn == thisOne) && (count < 256)) {
            pucIn++;
            count++;
        }
        if (count <= 3) {
            if (thisOne == 255) {
                *pucOut++ = 255;
                *pucOut++ = count - 1;
            } else {
                for (unsigned int i = 0; i < count; i++) *pucOut++ = thisOne;
            }
        } else {
            *pucOut++ = 255;
            *pucOut++ = count - 1;
            *pucOut++ = thisOne;
        }
    } while (pucIn != pucEnd);
    unsigned int rleSize = (unsigned int)(pucOut - rleCompressBuf.data());

    Compress(pDestination, pDestSize, rleCompressBuf.data(), rleSize);
    return 0;
}

int32_t Compression::DecompressLZXRLE(void* pDestination,
                                      unsigned int* pDestSize, void* pSource,
                                      unsigned int SrcSize) {
    std::lock_guard<std::mutex> lock(rleDecompressLock);

    unsigned int rleSize = *pDestSize;
    if (rleDecompressBuf.size() < rleSize) rleDecompressBuf.resize(rleSize);

    Decompress(rleDecompressBuf.data(), &rleSize, pSource, SrcSize);

    unsigned char* pucIn = rleDecompressBuf.data();
    unsigned char* pucEnd = pucIn + rleSize;
    unsigned char* pucOut = (unsigned char*)pDestination;

    while (pucIn != pucEnd) {
        unsigned char thisOne = *pucIn++;
        if (thisOne == 255) {
            unsigned int count = *pucIn++;
            if (count < 3) {
                count++;
                for (unsigned int i = 0; i < count; i++) *pucOut++ = 255;
            } else {
                count++;
                unsigned char data = *pucIn++;
                for (unsigned int i = 0; i < count; i++) *pucOut++ = data;
            }
        } else {
            *pucOut++ = thisOne;
        }
    }
    *pDestSize = (unsigned int)(pucOut - (unsigned char*)pDestination);
    return 0;
}

int32_t Compression::CompressRLE(void* pDestination, unsigned int* pDestSize,
                                 void* pSource, unsigned int SrcSize) {
    unsigned int rleSize;
    {
        std::lock_guard<std::mutex> lock(rleCompressLock);

        if (rleCompressBuf.size() < SrcSize * 2)
            rleCompressBuf.resize(SrcSize * 2);

        unsigned char* pucIn = (unsigned char*)pSource;
        unsigned char* pucEnd = pucIn + SrcSize;
        unsigned char* pucOut = rleCompressBuf.data();

        do {
            unsigned char thisOne = *pucIn++;
            unsigned int count = 1;
            while ((pucIn != pucEnd) && (*pucIn == thisOne) && (count < 256)) {
                pucIn++;
                count++;
            }
            if (count <= 3) {
                if (thisOne == 255) {
                    *pucOut++ = 255;
                    *pucOut++ = count - 1;
                } else {
                    for (unsigned int i = 0; i < count; i++)
                        *pucOut++ = thisOne;
                }
            } else {
                *pucOut++ = 255;
                *pucOut++ = count - 1;
                *pucOut++ = thisOne;
            }
        } while (pucIn != pucEnd);
        rleSize = (unsigned int)(pucOut - rleCompressBuf.data());
    }

    if (rleSize <= *pDestSize) {
        *pDestSize = rleSize;
        memcpy(pDestination, rleCompressBuf.data(), *pDestSize);
    } else {
        assert(false);
    }
    return 0;
}

int32_t Compression::DecompressRLE(void* pDestination, unsigned int* pDestSize,
                                   void* pSource, unsigned int SrcSize) {
    std::lock_guard<std::mutex> lock(rleDecompressLock);

    unsigned char* pucIn = (unsigned char*)pSource;
    unsigned char* pucEnd = pucIn + SrcSize;
    unsigned char* pucOut = (unsigned char*)pDestination;

    while (pucIn != pucEnd) {
        unsigned char thisOne = *pucIn++;
        if (thisOne == 255) {
            unsigned int count = *pucIn++;
            if (count < 3) {
                count++;
                for (unsigned int i = 0; i < count; i++) *pucOut++ = 255;
            } else {
                count++;
                unsigned char data = *pucIn++;
                for (unsigned int i = 0; i < count; i++) *pucOut++ = data;
            }
        } else {
            *pucOut++ = thisOne;
        }
    }
    *pDestSize = (unsigned int)(pucOut - (unsigned char*)pDestination);
    return 0;
}

int32_t Compression::Compress(void* pDestination, unsigned int* pDestSize,
                              void* pSource, unsigned int SrcSize) {
    size_t destSize = (size_t)(*pDestSize);
    int res = ::compress((Bytef*)pDestination, (uLongf*)&destSize,
                         (Bytef*)pSource, SrcSize);
    *pDestSize = (unsigned int)destSize;
    return ((res == Z_OK) ? 0 : -1);
}

int32_t Compression::Decompress(void* pDestination, unsigned int* pDestSize,
                                void* pSource, unsigned int SrcSize) {
    size_t destSize = (size_t)(*pDestSize);
    int res = ::uncompress((Bytef*)pDestination, (uLongf*)&destSize,
                           (Bytef*)pSource, SrcSize);
    *pDestSize = (unsigned int)destSize;
    return ((res == Z_OK) ? 0 : -1);
}

Compression::Compression() {
    m_localDecompressType = eCompressionType_ZLIBRLE;
    m_decompressType = m_localDecompressType;
}

Compression::~Compression() {}

void Compression::SetDecompressionType(ESavePlatform platform) {
    switch (platform) {
        case SAVE_FILE_PLATFORM_XBONE:
        case SAVE_FILE_PLATFORM_PS4:
        case SAVE_FILE_PLATFORM_PSVITA:
        case SAVE_FILE_PLATFORM_WIN64:
            getCompression()->SetDecompressionType(eCompressionType_ZLIBRLE);
            break;
        default:
            assert(0);
            break;
    }
}
