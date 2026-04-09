#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "../IPlatformFilesystem.h"

// Standard filesystem implementation for desktop platforms.
class StdFilesystem : public IPlatformFilesystem {
public:
    ReadResult readFile(const std::filesystem::path& path, void* buffer,
                        std::size_t capacity) override;

    ReadResult readFileSegment(const std::filesystem::path& path,
                               std::size_t offset, void* buffer,
                               std::size_t bytesToRead) override;

    std::vector<std::uint8_t> readFileToVec(
        const std::filesystem::path& path) override;

    bool writeFile(const std::filesystem::path& path, const void* buffer,
                   std::size_t bytesToWrite) override;

    bool exists(const std::filesystem::path& path) override;

    std::size_t fileSize(const std::filesystem::path& path) override;

    std::filesystem::path getBasePath() override;

    std::filesystem::path getUserDataPath() override;
};