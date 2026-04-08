#include "StdFilesystem.h"

#include <fstream>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace platform_internal {
IPlatformFilesystem& PlatformFilesystem_get() {
    static StdFilesystem instance;
    return instance;
}
}

IPlatformFilesystem::ReadResult StdFilesystem::readFile(const std::filesystem::path& path,
                                   void* buffer, std::size_t capacity) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) return {ReadStatus::NotFound, 0, 0};
    if (size > capacity)
        return {ReadStatus::TooLarge, 0, static_cast<std::size_t>(size)};

    std::ifstream f(path, std::ios::binary);
    if (!f) return {ReadStatus::NotFound, 0, 0};
    f.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
    auto read = static_cast<std::size_t>(f.gcount());
    return {f ? ReadStatus::Ok : ReadStatus::ReadError, read,
            static_cast<std::size_t>(size)};
}

IPlatformFilesystem::ReadResult StdFilesystem::readFileSegment(const std::filesystem::path& path,
                                          std::size_t offset, void* buffer,
                                          std::size_t bytesToRead) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) return {ReadStatus::NotFound, 0, 0};
    if (offset + bytesToRead > size)
        return {ReadStatus::TooLarge, 0, static_cast<std::size_t>(size)};

    std::ifstream f(path, std::ios::binary);
    if (!f) return {ReadStatus::NotFound, 0, 0};
    f.seekg(static_cast<std::streamoff>(offset));
    f.read(static_cast<char*>(buffer),
           static_cast<std::streamsize>(bytesToRead));
    auto read = static_cast<std::size_t>(f.gcount());
    return {f ? ReadStatus::Ok : ReadStatus::ReadError, read,
            static_cast<std::size_t>(size)};
}

std::vector<std::uint8_t> StdFilesystem::readFileToVec(
    const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) return {};

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    f.read(reinterpret_cast<char*>(data.data()),
           static_cast<std::streamsize>(size));
    return data;
}

bool StdFilesystem::writeFile(const std::filesystem::path& path,
                              const void* buffer, std::size_t bytesToWrite) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(static_cast<const char*>(buffer),
            static_cast<std::streamsize>(bytesToWrite));
    return f.good();
}

bool StdFilesystem::exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

std::size_t StdFilesystem::fileSize(const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : static_cast<std::size_t>(size);
}

std::filesystem::path StdFilesystem::getBasePath() {
#if defined(__linux__)
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        return std::filesystem::path(buf).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path StdFilesystem::getUserDataPath() { return getBasePath(); }