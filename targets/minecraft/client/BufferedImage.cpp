#include "minecraft/client/BufferedImage.h"

#include <string.h>

#include <cstdint>
#include <string>
#include <vector>

#include "platform/PlatformTypes.h"
#include "minecraft/IGameServices.h"
#include "platform/fs/fs.h"
#include "platform/renderer/renderer.h"
#include "util/StringHelpers.h"

BufferedImage::BufferedImage(int width, int height, int type) {
    data[0] = new int[width * height];

    for (int i = 1; i < 10; i++) {
        data[i] = nullptr;
    }
    this->width = width;
    this->height = height;
}

void BufferedImage::ByteFlip4(unsigned int& data) {
    data = (data >> 24) | ((data >> 8) & 0x0000ff00) |
           ((data << 8) & 0x00ff0000) | (data << 24);
}
// Loads a bitmap into a buffered image - only currently supports the 2 types of
// 32-bit image that we've made so far and determines which of these is which by
// the compression method. Compression method 3 is a 32-bit image with only
// 24-bits used (ie no alpha channel) whereas method 0 is a full 32-bit image
// with a valid alpha channel.

// 4jcraft: mostly rewrote this function
BufferedImage::BufferedImage(const std::string& File, bool filenameHasExtension,
                             bool bTitleUpdateTexture,
                             const std::string& drive) {
    int32_t hr = -1;
    std::string filePath = File;

    for (size_t i = 0; i < filePath.length(); ++i) {
        if (filePath[i] == '\\') filePath[i] = '/';
    }
    for (int l = 0; l < 10; l++) data[l] = nullptr;

    std::string baseName = filePath;
    if (!filenameHasExtension) {
        if (baseName.size() > 4 &&
            baseName.substr(baseName.size() - 4) == ".png") {
            baseName = baseName.substr(0, baseName.size() - 4);
        }
    }

    while (!baseName.empty() && (baseName[0] == '/' || baseName[0] == '\\'))
        baseName = baseName.substr(1);
    if (baseName.find("res/") == 0) baseName = baseName.substr(4);

    std::string exeDir = PlatformFilesystem.getBasePath().string();

    for (int l = 0; l < 10; l++) {
        std::string mipSuffix =
            (l != 0) ? "MipMapLevel" + toWString<int>(l + 1) : "";
        std::string fileName = baseName + mipSuffix + ".png";
        std::string finalPath;
        bool foundOnDisk = false;

        std::vector<std::string> searchPaths = {
            exeDir + "/Common/res/TitleUpdate/res/" + fileName,
            exeDir + "/Common/res/" + fileName,
            exeDir + "/Common/Media/Graphics/" + fileName,
            exeDir + "/Common/Media/font/" + fileName,
            exeDir + "/Common/res/font/" + fileName,
            exeDir + "/Common/Media/" + fileName};

        for (auto& attempt : searchPaths) {
            size_t p;
            while ((p = attempt.find("//")) != std::string::npos)
                attempt.replace(p, 2, "/");
            if (PlatformFilesystem.exists(attempt)) {
                finalPath = attempt;
                foundOnDisk = true;
                break;
            }
        }

        D3DXIMAGE_INFO ImageInfo;
        memset(&ImageInfo, 0, sizeof(D3DXIMAGE_INFO));

        if (foundOnDisk) {
            std::string nativePath = std::filesystem::path(finalPath).string();
            hr = PlatformRenderer.LoadTextureData(nativePath.c_str(),
                                                  &ImageInfo, &data[l]);
        } else {
            std::string archiveKey = "res/" + fileName;
            if (gameServices().hasArchiveFile(archiveKey)) {
                std::vector<uint8_t> ba =
                    gameServices().getArchiveFile(archiveKey);
                hr = PlatformRenderer.LoadTextureData(ba.data(), ba.size(),
                                                      &ImageInfo, &data[l]);
            }
        }

        if (hr == 0) {
            if (l == 0) {
                width = ImageInfo.Width;
                height = ImageInfo.Height;
            }
        } else {
            if (l == 0) {
                // safety dummy to prevent crash
                width = 1;
                height = 1;
                data[0] = new int[1];
                data[0][0] = 0xFFFF00FF;
            }
            break;
        }
    }
}
BufferedImage::BufferedImage() {
    for (int l = 0; l < 10; l++) data[l] = nullptr;
    width = 0;
    height = 0;
}

bool BufferedImage::loadMipmapPng(int level, std::uint8_t* bytes,
                                  std::uint32_t numBytes) {
    if (level < 0 || level >= 10 || bytes == nullptr || numBytes == 0) {
        return false;
    }
    D3DXIMAGE_INFO ImageInfo;
    int32_t hr = PlatformRenderer.LoadTextureData(bytes, numBytes, &ImageInfo,
                                                  &data[level]);
    if (hr != 0) {
        return false;
    }
    if (level == 0) {
        width = ImageInfo.Width;
        height = ImageInfo.Height;
    }
    return true;
}

BufferedImage::BufferedImage(std::uint8_t* pbData, std::uint32_t dataBytes) {
    for (int l = 0; l < 10; l++) {
        data[l] = nullptr;
    }

    D3DXIMAGE_INFO ImageInfo;
    memset(&ImageInfo, 0, sizeof(D3DXIMAGE_INFO));
    int32_t hr = PlatformRenderer.LoadTextureData(pbData, dataBytes, &ImageInfo,
                                                  &data[0]);

    if (hr == 0) {
        width = ImageInfo.Width;
        height = ImageInfo.Height;
    } else {
        gameServices().fatalLoadError();
    }
}

BufferedImage::~BufferedImage() {
    for (int i = 0; i < 10; i++) {
        delete[] data[i];
    }
}

int BufferedImage::getWidth() { return width; }

int BufferedImage::getHeight() { return height; }

void BufferedImage::getRGB(int startX, int startY, int w, int h,
                           std::vector<int>& out, int offset, int scansize,
                           int level) {
    int ww = width >> level;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            out[y * scansize + offset + x] =
                data[level][startX + x + ww * (startY + y)];
        }
    }
}

int* BufferedImage::getData() { return data[0]; }

int* BufferedImage::getData(int level) { return data[level]; }

Graphics* BufferedImage::getGraphics() { return nullptr; }

// Returns the transparency. Returns either OPAQUE, BITMASK, or TRANSLUCENT.
// Specified by:
// getTransparency in interface Transparency
// Returns:
// the transparency of this BufferedImage.
int BufferedImage::getTransparency() {
    // TODO - 4J Implement?
    return 0;
}

// Returns a subimage defined by a specified rectangular region. The returned
// BufferedImage shares the same data array as the original image. Parameters:
// x, y - the coordinates of the upper-left corner of the specified rectangular
// region w - the width of the specified rectangular region h - the height of
// the specified rectangular region Returns: a BufferedImage that is the
// subimage of this BufferedImage.
BufferedImage* BufferedImage::getSubimage(int x, int y, int w, int h) {
    // TODO - 4J Implement

    BufferedImage* img = new BufferedImage(w, h, 0);

    // 4jcraft: Copy pixel data directly into img->data[0].
    // The old arrayWithLength.h (custom vector impl) was a non-owning wrapper,
    // std::vector copies so we write to the raw array directly instead.
    int srcW = width;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            img->data[0][row * w + col] = data[0][(y + row) * srcW + (x + col)];
        }
    }

    int level = 1;
    while (level < 10 && getData(level) != nullptr) {
        int ww = w >> level;
        int hh = h >> level;
        int xx = x >> level;
        int yy = y >> level;
        int srcW = width >> level;
        img->data[level] = new int[ww * hh];
        for (int row = 0; row < hh; row++) {
            for (int col = 0; col < ww; col++) {
                img->data[level][row * ww + col] =
                    data[level][(yy + row) * srcW + (xx + col)];
            }
        }
        ++level;
    }

    return img;
}

void BufferedImage::preMultiplyAlpha() {
    int* curData = data[0];

    int cur = 0;
    int alpha = 0;
    int r = 0;
    int g = 0;
    int b = 0;

    int total = width * height;
    // why was it unsigned??
    for (int i = 0; i < total; ++i) {
        cur = curData[i];
        alpha = (cur >> 24) & 0xff;
        r = ((cur >> 16) & 0xff) * (float)alpha / 255;
        g = ((cur >> 8) & 0xff) * (float)alpha / 255;
        b = (cur & 0xff) * (float)alpha / 255;

        curData[i] = (r << 16) | (g << 8) | (b) | (alpha << 24);
    }
}
