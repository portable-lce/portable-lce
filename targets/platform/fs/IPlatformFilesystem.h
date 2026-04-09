#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

// Platform-agnostic file I/O interface.
class IPlatformFilesystem {
public:
    enum class ReadStatus {
        Ok,
        NotFound,
        TooLarge,
        ReadError,
    };

    struct ReadResult {
        ReadStatus status;
        std::size_t bytesRead;
        std::size_t fileSize;
    };

    virtual ~IPlatformFilesystem() = default;

    // Read an entire file into a caller-provided buffer.
    [[nodiscard]] virtual ReadResult readFile(const std::filesystem::path& path,
                                              void* buffer,
                                              std::size_t capacity) = 0;

    // Read a segment of a file.
    [[nodiscard]] virtual ReadResult readFileSegment(
        const std::filesystem::path& path, std::size_t offset, void* buffer,
        std::size_t bytesToRead) = 0;

    // Read an entire file into a vector.
    [[nodiscard]] virtual std::vector<std::uint8_t> readFileToVec(
        const std::filesystem::path& path) = 0;

    // Write a buffer to a file, creating or overwriting.
    virtual bool writeFile(const std::filesystem::path& path,
                           const void* buffer, std::size_t bytesToWrite) = 0;

    // Check if a path exists.
    [[nodiscard]] virtual bool exists(const std::filesystem::path& path) = 0;

    // Get file size without reading.
    [[nodiscard]] virtual std::size_t fileSize(
        const std::filesystem::path& path) = 0;

    // Base path for game assets.
    [[nodiscard]] virtual std::filesystem::path getBasePath() = 0;

    // Path for user data (saves, config).
    [[nodiscard]] virtual std::filesystem::path getUserDataPath() = 0;
};
