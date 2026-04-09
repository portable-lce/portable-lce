#pragma once

#include <map>
#include <string>

class IntBuffer;
class Options;
class Textures;
class ResourceLocation;
class Random;

class Font {
private:
    int* charWidths;

public:
    int fontTexture;
    Random* random;

private:
    int colors[32];  // RGB colors for formatting

    Textures* textures;

    float xPos;
    float yPos;

    bool enforceUnicodeSheet;  // use unicode sheet for ascii
    bool bidirectional;        // use bidi to flip strings

    int m_cols;                           // Number of columns in font sheet
    int m_rows;                           // Number of rows in font sheet
    int m_charWidth;                      // Maximum character width
    int m_charHeight;                     // Maximum character height
    ResourceLocation* m_textureLocation;  // Texture
    std::map<int, int> m_charMap;

public:
    Font(Options* options, const std::string& name, Textures* textures,
         bool enforceUnicode, ResourceLocation* textureLocation, int cols,
         int rows, int charWidth, int charHeight,
         unsigned short charMap[] = nullptr);
    // 4J Stu - This dtor clashes with one in xui! We never delete these anyway
    // so take it out for now. Can go back when we have got rid of XUI
    ~Font();
    void renderFakeCB(IntBuffer* cb);  // 4J added

private:
    void renderCharacter(char c);  // 4J added

public:
    void drawShadow(const std::string& str, int x, int y, int color);
    void drawShadowWordWrap(const std::string& str, int x, int y, int w,
                            int color, int h);  // 4J Added h param
    void draw(const std::string& str, int x, int y, int color);
    /**
     * Reorders the string according to bidirectional levels. A bit expensive at
     * the moment.
     *
     * @param str
     * @return
     */
private:
    std::string reorderBidi(const std::string& str);

    void draw(const std::string& str, bool dropShadow);
    void draw(const std::string& str, int x, int y, int color, bool dropShadow);
    int MapCharacter(char c);      // 4J added
    bool CharacterExists(char c);  // 4J added

public:
    int width(const std::string& str);
    std::string sanitize(const std::string& str);
    void drawWordWrap(const std::string& string, int x, int y, int w, int col,
                      int h);  // 4J Added h param

private:
    void drawWordWrapInternal(const std::string& string, int x, int y, int w,
                              int col, int h);  // 4J Added h param

public:
    void drawWordWrap(const std::string& string, int x, int y, int w, int col,
                      bool darken, int h);  // 4J Added h param

private:
    void drawWordWrapInternal(const std::string& string, int x, int y, int w,
                              int col, bool darken, int h);  // 4J Added h param

public:
    int wordWrapHeight(const std::string& string, int w);
    void setEnforceUnicodeSheet(bool enforceUnicodeSheet);
    void setBidirectional(bool bidirectional);

    // 4J-PB - check for invalid player name - Japanese local name
    bool AllCharactersValid(const std::string& str);
};
