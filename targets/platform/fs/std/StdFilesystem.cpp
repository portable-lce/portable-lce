#include "StdFilesystem.h"

#include <fstream>

#if defined(__linux__)
#include <unistd.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace platform_internal {
IPlatformFilesystem& PlatformFilesystem_get() {
    static StdFilesystem instance;
    return instance;
}
}  // namespace platform_internal

IPlatformFilesystem::ReadResult StdFilesystem::readFile(
    const std::filesystem::path& path, void* buffer, std::size_t capacity) {
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

IPlatformFilesystem::ReadResult StdFilesystem::readFileSegment(
    const std::filesystem::path& path, std::size_t offset, void* buffer,
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
#if defined(_WIN32)
    wchar_t buf[4096];
    DWORD len = GetModuleFileNameW(nullptr, buf, sizeof(buf) / sizeof(wchar_t));
    if (len > 0) {
        return std::filesystem::path(buf).parent_path();
    }
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return std::filesystem::path(buf).parent_path();
    }
#elif defined(__linux__)
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path StdFilesystem::getUserDataPath() { return getBasePath(); }
