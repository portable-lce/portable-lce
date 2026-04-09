#include "Texture.h"

#include <string.h>

#include <cstdint>
#include <vector>

#include "TextureManager.h"
#include "java/Buffer.h"
#include "java/ByteBuffer.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/renderer/Rect2i.h"
#include "minecraft/util/Log.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

#define MAX_MIP_LEVELS 5

Texture::Texture(const std::string& name, int mode, int width, int height,
                 int depth, int wrapMode, int format, int minFilter,
                 int magFilter, bool mipMap) {
    _init(name, mode, width, height, depth, wrapMode, format, minFilter,
          magFilter, mipMap);
}

void Texture::_init(const std::string& name, int mode, int width, int height,
                    int depth, int wrapMode, int format, int minFilter,
                    int magFilter, bool mipMap) {
    this->name = name;
    this->mode = mode;
    this->width = width;
    this->height = height;
    this->depth = depth;
    this->format = format;
    this->minFilter = minFilter;
    this->magFilter = magFilter;
    this->wrapMode = wrapMode;
    immediateUpdate = false;
    m_bInitialised = false;
    for (int i = 0; i < 10; i++) {
        data[i] = nullptr;
    }

    rect = new Rect2i(0, 0, width, height);
    // 4J Removed 1D and 3D
    // if (height == 1 && depth == 1)
    //{
    //	type = GL_TEXTURE_1D;
    //}
    // else if(depth == 1)
    //{
    type = GL_TEXTURE_2D;
    //}
    // else
    //{
    //	type = GL_TEXTURE_3D;
    //}

    mipmapped = mipMap || (minFilter != GL_NEAREST && minFilter != GL_LINEAR) ||
                (magFilter != GL_NEAREST && magFilter != GL_LINEAR);
    m_iMipLevels = 1;

    if (mipmapped) {
        // 4J-PB - In the new XDK, the CreateTexture will fail if the number of
        // mipmaps is higher than the width & height passed in will allow!
        int iWidthMips = 1;
        int iHeightMips = 1;
        while ((8 << iWidthMips) < width) iWidthMips++;
        while ((8 << iHeightMips) < height) iHeightMips++;

        m_iMipLevels = (iWidthMips < iHeightMips) ? iWidthMips : iHeightMips;

        // TODO - The render libs currently limit max mip map levels to 5
        if (m_iMipLevels > MAX_MIP_LEVELS) m_iMipLevels = MAX_MIP_LEVELS;
    }

    if (mode != TM_CONTAINER) {
        glId = glGenTextures();

        glBindTexture(type, glId);
        glTexParameteri(type, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(type, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(type, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(type, GL_TEXTURE_WRAP_T, wrapMode);
    } else {
        glId = -1;
    }

    managerId = TextureManager::getInstance()->createTextureID();
}

void Texture::_init(const std::string& name, int mode, int width, int height,
                    int depth, int wrapMode, int format, int minFilter,
                    int magFilter, BufferedImage* image, bool mipMap) {
    _init(name, mode, width, height, depth, wrapMode, format, minFilter,
          magFilter, mipMap);
    if (image == nullptr) {
        if (width == -1 || height == -1) {
            valid = false;
        } else {
            std::vector<uint8_t> tempBytes =
                std::vector<uint8_t>(width * height * depth * 4);
            for (int index = 0; index < tempBytes.size(); index++) {
                tempBytes[index] = 0;
            }
            data[0] = ByteBuffer::allocateDirect(tempBytes.size());
            data[0]->clear();
            data[0]->put(tempBytes);
            data[0]->position(0)->limit(tempBytes.size());

            if (mipmapped) {
                for (unsigned int level = 1; level < m_iMipLevels; ++level) {
                    int ww = width >> level;
                    int hh = height >> level;

                    std::vector<uint8_t> tempBytes =
                        std::vector<uint8_t>(ww * hh * depth * 4);
                    for (int index = 0; index < tempBytes.size(); index++) {
                        tempBytes[index] = 0;
                    }

                    data[level] = ByteBuffer::allocateDirect(tempBytes.size());
                    data[level]->clear();
                    data[level]->put(tempBytes);
                    data[level]->position(0)->limit(tempBytes.size());
                }
            }

            if (immediateUpdate) {
                updateOnGPU();
            } else {
                updated = false;
            }
        }
    } else {
        valid = true;

        transferFromImage(image);

        if (mode != TM_CONTAINER) {
            updateOnGPU();
            immediateUpdate = false;
        }
    }
}

Texture::Texture(const std::string& name, int mode, int width, int height,
                 int wrapMode, int format, int minFilter, int magFilter,
                 BufferedImage* image, bool mipMap) {
    _init(name, mode, width, height, 1, wrapMode, format, minFilter, magFilter,
          image, mipMap);
}

Texture::Texture(const std::string& name, int mode, int width, int height,
                 int depth, int wrapMode, int format, int minFilter,
                 int magFilter, BufferedImage* image, bool mipMap) {
    _init(name, mode, width, height, depth, wrapMode, format, minFilter,
          magFilter, image, mipMap);
}

Texture::~Texture() {
    delete rect;

    for (int i = 0; i < 10; i++) {
        if (data[i] != nullptr) delete data[i];
    }

    if (glId >= 0) {
        glDeleteTextures(glId);
    }
}

const Rect2i* Texture::getRect() { return rect; }

void Texture::fill(const Rect2i* rect, int color) {
    // 4J Remove 3D
    // if (type == GL_TEXTURE_3D)
    //{
    //	return;
    //}

    Rect2i* myRect = new Rect2i(0, 0, width, height);
    myRect->intersect(rect);
    data[0]->position(0);
    for (int y = myRect->getY(); y < (myRect->getY() + myRect->getHeight());
         y++) {
        int line = y * width * 4;
        for (int x = myRect->getX(); x < (myRect->getX() + myRect->getWidth());
             x++) {
            data[0]->put(line + x * 4 + 0,
                         static_cast<std::uint8_t>((color >> 24) & 0x000000ff));
            data[0]->put(line + x * 4 + 1,
                         static_cast<std::uint8_t>((color >> 16) & 0x000000ff));
            data[0]->put(line + x * 4 + 2,
                         static_cast<std::uint8_t>((color >> 8) & 0x000000ff));
            data[0]->put(line + x * 4 + 3,
                         static_cast<std::uint8_t>((color >> 0) & 0x000000ff));
        }
    }
    delete myRect;

    if (immediateUpdate) {
        updateOnGPU();
    } else {
        updated = false;
    }
}

void Texture::writeAsBMP(const std::string& name) {
    // 4J Don't need
}

void Texture::writeAsPNG(const std::string& filename) {
    // 4J Don't need
}

void Texture::blit(int x, int y, Texture* source) { blit(x, y, source, false); }

void Texture::blit(int x, int y, Texture* source, bool rotated) {
    // 4J Remove 3D
    // if (type == GL_TEXTURE_3D)
    //{
    //	return;
    //}

    for (unsigned int level = 0; level < m_iMipLevels; ++level) {
        ByteBuffer* srcBuffer = source->getData(level);

        if (srcBuffer == nullptr) break;

        int yy = y >> level;
        int xx = x >> level;
        int hh = height >> level;
        int ww = width >> level;
        int shh = source->getHeight() >> level;
        int sww = source->getWidth() >> level;

        data[level]->position(0);
        srcBuffer->position(0);

        for (int srcY = 0; srcY < shh; srcY++) {
            int dstY = yy + srcY;
            int srcLine = srcY * sww * 4;
            int dstLine = dstY * ww * 4;

            if (rotated) {
                dstY = yy + (shh - srcY);
            }

            for (int srcX = 0; srcX < sww; srcX++) {
                int dstPos = dstLine + (srcX + xx) * 4;
                int srcPos = srcLine + srcX * 4;

                if (rotated) {
                    dstPos = (xx + srcX * ww * 4) + dstY * 4;
                }

                data[level]->put(dstPos + 0, srcBuffer->get(srcPos + 0));
                data[level]->put(dstPos + 1, srcBuffer->get(srcPos + 1));
                data[level]->put(dstPos + 2, srcBuffer->get(srcPos + 2));
                data[level]->put(dstPos + 3, srcBuffer->get(srcPos + 3));
            }
        }
        // Don't delete this, as it belongs to the source texture
        // delete srcBuffer;
        data[level]->position(ww * hh * 4);
    }

    if (immediateUpdate) {
        updateOnGPU();
    } else {
        updated = false;
    }
}

void Texture::transferFromBuffer(const std::vector<int>& buffer) {
    // if (depth == 1) {
    //     return;
    // }
    //  4jcraft - move pos out of loops
    data[0]->clear();
    // #if 0
    // 	int byteRemapRGBA[] = { 3, 0, 1, 2 };
    // 	int byteRemapBGRA[] = { 3, 2, 1, 0 };
    // #else
    int byteRemapRGBA[] = {0, 1, 2, 3};
    int byteRemapBGRA[] = {2, 1, 0, 3};
    // #endif
    int* byteRemap = ((format == TFMT_BGRA) ? byteRemapBGRA : byteRemapRGBA);

    int totalPixels = width * height * depth;

    for (int i = 0; i < totalPixels; i++) {
        int pixel = buffer[i];
        int offset = i * 4;

        data[0]->put(offset + byteRemap[0], (uint8_t)((pixel >> 24) & 0xff));
        data[0]->put(offset + byteRemap[1], (uint8_t)((pixel >> 16) & 0xff));
        data[0]->put(offset + byteRemap[2], (uint8_t)((pixel >> 8) & 0xff));
        data[0]->put(offset + byteRemap[3], (uint8_t)((pixel >> 0) & 0xff));

        data[0]->position(totalPixels * 4);
        data[0]->limit(totalPixels * 4);

        updateOnGPU();
    }

    /* for (int z = 0; z < depth; z++) {
        int plane = z * height * width * 4;
        for (int y = 0; y < height; y++) {
            int column = plane + y * width * 4;
            for (int x = 0; x < width; x++) {
                int texel = column + x * 4;
                data[0]->position(0);
                data[0]->put(texel + byteRemap[0],
                             (uint8_t)((buffer[texel >> 2] >> 24) & 0xff));
                data[0]->put(texel + byteRemap[1],
                             (uint8_t)((buffer[texel >> 2] >> 16) & 0xff));
                data[0]->put(texel + byteRemap[2],
                             (uint8_t)((buffer[texel >> 2] >> 8) & 0xff));
                data[0]->put(texel + byteRemap[3],
                             (uint8_t)((buffer[texel >> 2] >> 0) & 0xff));
            }
        }
    }

    data[0]->position(width * height * depth * 4);
    */

    updateOnGPU();
}

void Texture::transferFromImage(BufferedImage* image) {
    // 4J Remove 3D
    // if (type == GL_TEXTURE_3D)
    //{
    //	return;
    //}

    int imgWidth = image->getWidth();
    int imgHeight = image->getHeight();
    if (imgWidth > width || imgHeight > height) {
        // Minecraft::GetInstance().getLogger().warning("transferFromImage
        // called with a BufferedImage with dimensions (" + 	imgWidth + ", "
        // +
        // imgHeight + ") larger than the Texture dimensions (" + width +
        //	", " + height + "). Ignoring.");
        Log::info(
            "transferFromImage called with a BufferedImage with dimensions "
            "(%d, %d) larger than the Texture dimensions (%d, %d). Ignoring.\n",
            imgWidth, imgHeight, width, height);
        return;
    }

    // #if 0
    // 	int byteRemapRGBA[] = { 0, 1, 2, 3 };
    // 	int byteRemapBGRA[] = { 2, 1, 0, 3 };
    // #else
    int byteRemapRGBA[] = {3, 0, 1, 2};
    int byteRemapBGRA[] = {3, 2, 1, 0};
    // #endif
    int* byteRemap = ((format == TFMT_BGRA) ? byteRemapBGRA : byteRemapRGBA);

    std::vector<int> tempPixels = std::vector<int>(width * height);
    int transparency = image->getTransparency();
    image->getRGB(0, 0, width, height, tempPixels, 0, imgWidth);

    std::vector<uint8_t> tempBytes = std::vector<uint8_t>(width * height * 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int intIndex = y * width + x;
            int byteIndex = intIndex * 4;

            // Pull ARGB bytes into either RGBA or BGRA depending on format

            tempBytes[byteIndex + byteRemap[0]] =
                (uint8_t)((tempPixels[intIndex] >> 24) & 0xff);
            tempBytes[byteIndex + byteRemap[1]] =
                (uint8_t)((tempPixels[intIndex] >> 16) & 0xff);
            tempBytes[byteIndex + byteRemap[2]] =
                (uint8_t)((tempPixels[intIndex] >> 8) & 0xff);
            tempBytes[byteIndex + byteRemap[3]] =
                (uint8_t)((tempPixels[intIndex] >> 0) & 0xff);
        }
    }

    for (int i = 0; i < 10; i++) {
        if (data[i] != nullptr) {
            delete data[i];
            data[i] = nullptr;
        }
    }

    data[0] = ByteBuffer::allocateDirect(tempBytes.size());
    data[0]->clear();
    data[0]->put(tempBytes);
    data[0]->limit(tempBytes.size());

    if (mipmapped || image->getData(1) != nullptr) {
        mipmapped = true;
        for (unsigned int level = 1; level < MAX_MIP_LEVELS; ++level) {
            int ww = width >> level;
            int hh = height >> level;

            std::vector<uint8_t> tempBytes = std::vector<uint8_t>(ww * hh * 4);
            unsigned int* tempData = new unsigned int[ww * hh];

            if (image->getData(level)) {
                memcpy(tempData, image->getData(level), ww * hh * 4);
                for (int y = 0; y < hh; y++) {
                    for (int x = 0; x < ww; x++) {
                        int intIndex = y * ww + x;
                        int byteIndex = intIndex * 4;

                        // Pull ARGB bytes into either RGBA or BGRA depending on
                        // format

                        tempBytes[byteIndex + byteRemap[0]] =
                            (uint8_t)((tempData[intIndex] >> 24) & 0xff);
                        tempBytes[byteIndex + byteRemap[1]] =
                            (uint8_t)((tempData[intIndex] >> 16) & 0xff);
                        tempBytes[byteIndex + byteRemap[2]] =
                            (uint8_t)((tempData[intIndex] >> 8) & 0xff);
                        tempBytes[byteIndex + byteRemap[3]] =
                            (uint8_t)((tempData[intIndex] >> 0) & 0xff);
                    }
                }
            } else {
                int ow = width >> (level - 1);

                for (int x = 0; x < ww; x++)
                    for (int y = 0; y < hh; y++) {
                        int c0 = data[level - 1]->getInt(
                            ((x * 2 + 0) + (y * 2 + 0) * ow) * 4);
                        int c1 = data[level - 1]->getInt(
                            ((x * 2 + 1) + (y * 2 + 0) * ow) * 4);
                        int c2 = data[level - 1]->getInt(
                            ((x * 2 + 1) + (y * 2 + 1) * ow) * 4);
                        int c3 = data[level - 1]->getInt(
                            ((x * 2 + 0) + (y * 2 + 1) * ow) * 4);
                        // 4J - convert our RGBA texels to ARGB that crispBlend
                        // is expecting 4jcraft, added uint cast to pervent
                        // shift of neg int
                        c0 =
                            ((c0 >> 8) & 0x00ffffff) | ((unsigned int)c0 << 24);
                        c1 =
                            ((c1 >> 8) & 0x00ffffff) | ((unsigned int)c1 << 24);
                        c2 =
                            ((c2 >> 8) & 0x00ffffff) | ((unsigned int)c2 << 24);
                        c3 =
                            ((c3 >> 8) & 0x00ffffff) | ((unsigned int)c3 << 24);
                        int col =
                            crispBlend(crispBlend(c0, c1), crispBlend(c2, c3));
                        // 4J - and back from ARGB -> RGBA
                        // col = ( col << 8 ) | (( col >> 24 ) & 0xff);
                        // tempData[x + y * ww] = col;

                        int intIndex = y * ww + x;
                        int byteIndex = intIndex * 4;

                        // Pull ARGB bytes into either RGBA or BGRA depending on
                        // format

                        tempBytes[byteIndex + byteRemap[0]] =
                            (uint8_t)((col >> 24) & 0xff);
                        tempBytes[byteIndex + byteRemap[1]] =
                            (uint8_t)((col >> 16) & 0xff);
                        tempBytes[byteIndex + byteRemap[2]] =
                            (uint8_t)((col >> 8) & 0xff);
                        tempBytes[byteIndex + byteRemap[3]] =
                            (uint8_t)((col >> 0) & 0xff);
                    }
            }

            data[level] = ByteBuffer::allocateDirect(tempBytes.size());
            data[level]->clear();
            data[level]->put(tempBytes);
            data[level]->limit(tempBytes.size());
            delete[] tempData;
        }
    }

    if (immediateUpdate) {
        updateOnGPU();
    } else {
        updated = false;
    }
}

// 4J Kept from older versions for where we create mip-maps for levels that do
// not have pre-made graphics
int Texture::crispBlend(int c0, int c1) {
    int a0 = (int)(((c0 & 0xff000000) >> 24)) & 0xff;
    int a1 = (int)(((c1 & 0xff000000) >> 24)) & 0xff;

    int a = 255;
    if (a0 + a1 < 255) {
        a = 0;
        a0 = 1;
        a1 = 1;
    } else if (a0 > a1) {
        a0 = 255;
        a1 = 1;
    } else {
        a0 = 1;
        a1 = 255;
    }

    int r0 = ((c0 >> 16) & 0xff) * a0;
    int g0 = ((c0 >> 8) & 0xff) * a0;
    int b0 = ((c0) & 0xff) * a0;

    int r1 = ((c1 >> 16) & 0xff) * a1;
    int g1 = ((c1 >> 8) & 0xff) * a1;
    int b1 = ((c1) & 0xff) * a1;

    int r = (r0 + r1) / (a0 + a1);
    int g = (g0 + g1) / (a0 + a1);
    int b = (b0 + b1) / (a0 + a1);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

int Texture::getManagerId() { return managerId; }

int Texture::getGlId() { return glId; }

int Texture::getWidth() { return width; }

int Texture::getHeight() { return height; }

std::string Texture::getName() { return name; }

void Texture::setImmediateUpdate(bool immediateUpdate) {
    this->immediateUpdate = immediateUpdate;
}

void Texture::bind(int mipMapIndex) {
    // 4J Removed 3D
    // if (depth == 1)
    //{
    glEnable(GL_TEXTURE_2D);
    //}
    // else
    //{
    //	glEnable(GL_TEXTURE_3D);
    //}

    glActiveTexture(GL_TEXTURE0 + mipMapIndex);
    glBindTexture(type, glId);
    if (!updated) {
        updateOnGPU();
    }
}

void Texture::updateOnGPU() {
    data[0]->flip();
    if (mipmapped) {
        for (int level = 1; level < m_iMipLevels; level++) {
            if (data[level] == nullptr) break;

            data[level]->flip();
        }
    }
    // 4J remove 3D and 1D
    // if (height != 1 && depth != 1)
    //{
    //	glTexImage3D(type, 0, format, width, height, depth, 0, format,
    // GL_UNSIGNED_BYTE, data);
    //}
    // else if(height != 1)
    //{
    // 4J Added check so we can differentiate between which PlatformRenderer
    // function to call
    if (!m_bInitialised) {
        PlatformRenderer.TextureSetTextureLevels(m_iMipLevels);  // 4J added

        PlatformRenderer.TextureData(
            width, height, data[0]->getBuffer(), 0,
            IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw);

        if (mipmapped) {
            for (int level = 1; level < m_iMipLevels; level++) {
                int levelWidth = width >> level;
                int levelHeight = height >> level;

                PlatformRenderer.TextureData(
                    levelWidth, levelHeight, data[level]->getBuffer(), level,
                    IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw);
            }
        }

        m_bInitialised = true;
    } else {
        PlatformRenderer.TextureDataUpdate(0, 0, width, height,
                                           data[0]->getBuffer(), 0);

        if (mipmapped) {
            if (PlatformRenderer.TextureGetTextureLevels() > 1) {
                for (int level = 1; level < m_iMipLevels; level++) {
                    int levelWidth = width >> level;
                    int levelHeight = height >> level;

                    PlatformRenderer.TextureDataUpdate(
                        0, 0, levelWidth, levelHeight, data[level]->getBuffer(),
                        level);
                }
            }
        }
    }
    // glTexImage2D(type, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE,
    // data);
    //}
    // else
    //{
    //	glTexImage1D(type, 0, format, width, 0, format, GL_UNSIGNED_BYTE, data);
    //}
    updated = true;
}

ByteBuffer* Texture::getData(unsigned int level) { return data[level]; }
