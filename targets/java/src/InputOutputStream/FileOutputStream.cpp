#include "java/InputOutputStream/FileOutputStream.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "java/File.h"

// Creates a file output stream to write to the file represented by the
// specified File object. A new FileDescriptor object is created to represent
// this file connection. First, if there is a security manager, its checkWrite
// method is called with the path represented by the file argument as its
// argument.
//
// If the file exists but is a directory rather than a regular file, does not
// exist but cannot be created, or cannot be opened for any other reason then a
// FileNotFoundException is thrown.
//
// Parameters:
// file - the file to be opened for writing.
FileOutputStream::FileOutputStream(const File& file) : m_fileHandle(nullptr) {
    if (file.exists() && file.isDirectory()) {
        // TODO 4J Stu - FileNotFoundException
        return;
    }

#if defined(_WIN32)
    m_fileHandle = _wfopen(file.getPath().c_str(), "wb");
#else
    const std::string nativePath =
        std::filesystem::path(file.getPath()).string();
    m_fileHandle = std::fopen(nativePath.c_str(), "wb");
#endif

    if (m_fileHandle == nullptr) {
        // TODO 4J Stu - Any form of error/exception handling
        perror("FileOutputStream::FileOutputStream");
    }
}

FileOutputStream::~FileOutputStream() {
    if (m_fileHandle != nullptr) {
        std::fclose(m_fileHandle);
    }
}

// Writes the specified byte to this file output stream. Implements the write
// method of OutputStream. Parameters: b - the byte to be written.
void FileOutputStream::write(unsigned int b) {
    if (m_fileHandle == nullptr) {
        return;
    }

    std::uint8_t value = (std::uint8_t)b;
    const size_t numberOfBytesWritten = std::fwrite(&value, 1, 1, m_fileHandle);
    const int result = std::ferror(m_fileHandle);

    if (result != 0) {
        // TODO 4J Stu - Some kind of error handling
    } else if (numberOfBytesWritten == 0) {
        // File pointer is past the end of the file
    }
}

// Writes b.size() bytes from the specified byte array to this file output
// stream. Parameters: b - the data.
void FileOutputStream::write(const std::vector<uint8_t>& b) {
    if (m_fileHandle == nullptr) {
        return;
    }

    const size_t numberOfBytesWritten =
        std::fwrite(b.data(), 1, b.size(), m_fileHandle);
    const int result = std::ferror(m_fileHandle);

    if (result != 0) {
        // TODO 4J Stu - Some kind of error handling
    } else if (numberOfBytesWritten == 0 || numberOfBytesWritten != b.size()) {
        // File pointer is past the end of the file
    }
}

// Writes len bytes from the specified byte array starting at offset off to this
// file output stream. Parameters: b - the data. off - the start offset in the
// data. len - the number of bytes to write.
void FileOutputStream::write(const std::vector<uint8_t>& b, unsigned int offset,
                             unsigned int length) {
    // 4J Stu - We don't want to write any more than the array buffer holds
    assert(length <= (b.size() - offset));

    if (m_fileHandle == nullptr) {
        return;
    }

    const size_t numberOfBytesWritten =
        std::fwrite(&b[offset], 1, length, m_fileHandle);
    const int result = std::ferror(m_fileHandle);

    if (result != 0) {
        // TODO 4J Stu - Some kind of error handling
    } else if (numberOfBytesWritten == 0 || numberOfBytesWritten != length) {
        // File pointer is past the end of the file
    }
}
//
// Closes this file output stream and releases any system resources associated
// with this stream. This file output stream may no longer be used for writing
// bytes. If this stream has an associated channel then the channel is closed as
// well.
void FileOutputStream::close() {
    if (m_fileHandle == nullptr) {
        return;
    }

    int result = std::fclose(m_fileHandle);
    if (result != 0) {
        // TODO 4J Stu - Some kind of error handling
    }

    // Stop the dtor from trying to close it again
    m_fileHandle = nullptr;
}

void FileOutputStream::flush() {
    if (m_fileHandle != nullptr) {
        std::fflush(m_fileHandle);
    }
}
