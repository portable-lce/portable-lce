
#include "java/InputOutputStream/FileInputStream.h"

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "java/File.h"

namespace {
int64_t FileTell(std::FILE* file) {
#if defined(_WIN32)
    return _ftelli64(file);
#else
    return static_cast<int64_t>(ftello(file));
#endif
}

bool FileSeek(std::FILE* file, int64_t offset, int origin) {
#if defined(_WIN32)
    return _fseeki64(file, offset, origin) == 0;
#else
    return fseeko(file, static_cast<off_t>(offset), origin) == 0;
#endif
}
}  // namespace

// Creates a FileInputStream by opening a connection to an actual file, the file
// named by the File object file in the file system. A new FileDescriptor object
// is created to represent this file connection. First, if there is a security
// manager, its checkRead method is called with the path represented by the file
// argument as its argument.
//
// If the named file does not exist, is a directory rather than a regular file,
// or for some other reason cannot be opened for reading then a
// FileNotFoundException is thrown.
//
// Parameters:
// file - the file to be opened for reading.
// Throws:
// FileNotFoundException - if the file does not exist, is a directory rather
// than a regular file, or for some other reason cannot be opened for reading.
// SecurityException - if a security manager exists and its checkRead method
// denies read access to the file.
FileInputStream::FileInputStream(const File& file) : m_fileHandle(nullptr) {
    const std::string nativePath =
        std::filesystem::path(file.getPath()).string();
    m_fileHandle = std::fopen(nativePath.c_str(), "rb");

    if (m_fileHandle == nullptr) {
        assert(0);
    }
}

FileInputStream::~FileInputStream() {
    if (m_fileHandle != nullptr) {
        std::fclose(m_fileHandle);
    }
}

// Reads a byte of data from this input stream. This method blocks if no input
// is yet available. Returns: the next byte of data, or -1 if the end of the
// file is reached.
int FileInputStream::read() {
    if (m_fileHandle == nullptr) {
        return -1;
    }

    std::uint8_t byteRead = static_cast<std::uint8_t>(0);
    const size_t numberOfBytesRead = std::fread(&byteRead, 1, 1, m_fileHandle);

    if (std::ferror(m_fileHandle) != 0) {
        assert(0);
    } else if (numberOfBytesRead == 0) {
        // File pointer is past the end of the file
        return -1;
    }

    return static_cast<int>(byteRead);
}

// Reads up to b.size() bytes of data from this input stream into an array of
// bytes. This method blocks until some input is available. Parameters: b - the
// buffer into which the data is read. Returns: the total number of bytes read
// into the buffer, or -1 if there is no more data because the end of the file
// has been reached.
int FileInputStream::read(std::vector<uint8_t>& b) {
    if (m_fileHandle == nullptr) {
        return -1;
    }

    const size_t numberOfBytesRead =
        std::fread(b.data(), 1, b.size(), m_fileHandle);

    if (std::ferror(m_fileHandle) != 0) {
        assert(0);
    } else if (numberOfBytesRead == 0) {
        // File pointer is past the end of the file
        return -1;
    }

    return numberOfBytesRead;
}

// Reads up to len bytes of data from this input stream into an array of bytes.
// If len is not zero, the method blocks until some input is available;
// otherwise, no bytes are read and 0 is returned. Parameters: b - the buffer
// into which the data is read. off - the start offset in the destination array
// b len - the maximum number of bytes read. Returns: the total number of bytes
// read into the buffer, or -1 if there is no more data because the end of the
// file has been reached.
int FileInputStream::read(std::vector<uint8_t>& b, unsigned int offset,
                          unsigned int length) {
    // 4J Stu - We don't want to read any more than the array buffer can hold
    assert(length <= (b.size() - offset));

    if (m_fileHandle == nullptr) {
        return -1;
    }

    const size_t numberOfBytesRead =
        std::fread(&b[offset], 1, length, m_fileHandle);

    if (std::ferror(m_fileHandle) != 0) {
        assert(0);
    } else if (numberOfBytesRead == 0) {
        // File pointer is past the end of the file
        return -1;
    }

    return numberOfBytesRead;
}

// Closes this file input stream and releases any system resources associated
// with the stream. If this stream has an associated channel then the channel is
// closed as well.
void FileInputStream::close() {
    if (m_fileHandle == nullptr) {
        // printf("\n\nFileInputStream::close - TRYING TO CLOSE AN INVALID FILE
        // void*\n\n");
        return;
    }

    int result = std::fclose(m_fileHandle);

    if (result != 0) {
        // TODO 4J Stu - Some kind of error handling
    }

    // Stop the dtor from trying to close it again
    m_fileHandle = nullptr;
}

// Skips n bytes of input from this input stream. Fewer bytes might be skipped
// if the end of the input stream is reached. The actual number k of bytes to be
// skipped is equal to the smaller of n and count-pos. The value k is added into
// pos and k is returned. Overrides: skip in class InputStream Parameters: n -
// the number of bytes to be skipped. Returns: the actual number of bytes
// skipped.
int64_t FileInputStream::skip(int64_t n) {
    if (m_fileHandle == nullptr || n <= 0) {
        return 0;
    }

    const int64_t start = FileTell(m_fileHandle);
    if (start < 0) {
        return 0;
    }

    if (!FileSeek(m_fileHandle, 0, SEEK_END)) {
        return 0;
    }

    const int64_t end = FileTell(m_fileHandle);
    if (end < 0) {
        return 0;
    }

    const int64_t offset = std::min(n, std::max<int64_t>(0, end - start));
    const int64_t target = start + offset;
    if (!FileSeek(m_fileHandle, target, SEEK_SET)) {
        return 0;
    }

    return offset;
}
