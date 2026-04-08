#pragma once

#include "platform/renderer/renderer.h"
#include "platform/stubs.h"

#include <format>
#include <string>
#include <vector>

class Rect2i;
class ByteBuffer;
class BufferedImage;

class Texture {
public:
    static const int WM_WRAP = GL_REPEAT;
    static const int WM_CLAMP = GL_CLAMP;
    static const int WM_MIRROR = 0;  // GL_MIRRORED_REPEAT;

    static const int TFMT_RGBA = GL_RGBA;
    static const int TFMT_BGRA = GL_BGRA;

    static const int TFLT_NEAREST = GL_NEAREST;
    static const int TFLT_LINEAR = GL_LINEAR;
    static const int TFLT_LINEAR_MIP_NEAREST = 0;  // GL_LINEAR_MIPMAP_NEAREST;
    static const int TFLT_LINEAR_MIP_LINEAR = 0;   // GL_LINEAR_MIPMAP_LINEAR;
    static const int TFLT_NEAREST_MIP_NEAREST =
        0;  // GL_NEAREST_MIPMAP_NEAREST;
    static const int TFLT_NEAREST_MIP_LINEAR = GL_NEAREST_MIPMAP_LINEAR;

    static const int TM_STATIC = 0;
    static const int TM_DYNAMIC = 1;
    static const int TM_CONTAINER = 2;

private:
    int glId;
    int managerId;

    // Indicates certain aspects of this texture's behavior in terms of how
    // tightly it is bound, conceptually. A static texture is loaded once,
    // uploaded to the GPU, and discarded CPU-side. A dynamic texture is kept on
    // both the CPU and GPU, as it will likely be dynamically updated on the
    // CPU. A container texture exists only to keep the data on the CPU, usually
    // for later combination into a larger texture via the Stitcher class.
    int mode;

    int width;
    int height;
    int depth;
    int format;
    int type;
    int minFilter;
    int magFilter;
    int wrapMode;
    bool mipmapped;
    std::string name;

    Rect2i* rect;

    bool valid;
    bool immediateUpdate;
    bool updated;
    int m_iMipLevels;
    ByteBuffer* data[10];  // Arrays for mipmaps - nullptr if not used

public:
    bool m_bInitialised;  // 4J Added

    ~Texture();

private:
    Texture(const std::string& name, int mode, int width, int height,
            int depth, int wrapMode, int format, int minFilter, int magFilter,
            bool mipMap = true);

    void _init(const std::string& name, int mode, int width, int height,
               int depth, int wrapMode, int format, int minFilter,
               int magFilter, bool mipMap);
    void _init(const std::string& name, int mode, int width, int height,
               int depth, int wrapMode, int format, int minFilter,
               int magFilter, BufferedImage* image, bool mipMap);

public:
    Texture(const std::string& name, int mode, int width, int height,
            int wrapMode, int format, int minFilter, int magFilter,
            BufferedImage* image, bool mipMap = true);
    Texture(const std::string& name, int mode, int width, int height,
            int depth, int wrapMode, int format, int minFilter, int magFilter,
            BufferedImage* image, bool mipMap = true);

    const Rect2i* getRect();
    void fill(const Rect2i* rect, int color);
    void writeAsBMP(const std::string& name);
    void writeAsPNG(const std::string& filename);
    void blit(int x, int y, Texture* source);
    void blit(int x, int y, Texture* source, bool rotated);
    void transferFromBuffer(const std::vector<int>& buffer);
    void transferFromImage(BufferedImage* image);
    int getManagerId();
    int getGlId();
    int getWidth();
    int getHeight();
    std::string getName();
    void setImmediateUpdate(bool immediateUpdate);
    void bind(int mipMapIndex);
    void updateOnGPU();
    ByteBuffer* getData(unsigned int level = 0);

    static int crispBlend(int c0, int c1);
};