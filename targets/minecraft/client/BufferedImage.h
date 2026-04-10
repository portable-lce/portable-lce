#pragma once
#include <cstdint>
#include <string>
#include <vector>

class Graphics;

class BufferedImage {
private:
    int* data[10];  // Arrays for mipmaps - nullptr if not used
    int width;
    int height;
    void ByteFlip4(unsigned int& data);  // 4J added
public:
    static const int TYPE_INT_ARGB = 0;
    static const int TYPE_INT_RGB = 1;
    BufferedImage();  // empty image; fill with loadMipmapPng()
    BufferedImage(int width, int height, int type);
    BufferedImage(const std::string& File, bool filenameHasExtension = false,
                  bool bTitleUpdateTexture = false,
                  const std::string& drive = "");                  // 4J added
    BufferedImage(std::uint8_t* pbData, std::uint32_t dataBytes);  // 4J added
    ~BufferedImage();

    // Decode `numBytes` of PNG-encoded texture data into mipmap slot
    // `level`. The first call (level == 0) populates width/height.
    // Returns true on success.
    bool loadMipmapPng(int level, std::uint8_t* bytes, std::uint32_t numBytes);

    int getWidth();
    int getHeight();
    void getRGB(int startX, int startY, int w, int h, std::vector<int>& out,
                int offset, int scansize,
                int level = 0);  // 4J Added level param
    int* getData();              // 4J added
    int* getData(int level);     // 4J added
    Graphics* getGraphics();
    int getTransparency();
    BufferedImage* getSubimage(int x, int y, int w, int h);

    void preMultiplyAlpha();
};
