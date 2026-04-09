#include "Font.h"

#include <string.h>

#include <utility>
#include <vector>

#include "java/Random.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Options.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/resources/ResourceLocation.h"
#include "platform/renderer/renderer.h"
#include "platform/stubs.h"
#include "util/StringHelpers.h"

Font::Font(Options* options, const std::string& name, Textures* textures,
           bool enforceUnicode, ResourceLocation* textureLocation, int cols,
           int rows, int charWidth, int charHeight,
           unsigned short charMap[] /* = nullptr */)
    : textures(textures) {
    int charC = cols * rows;  // Number of characters in the font

    charWidths = new int[charC];

    // 4J - added initialisers
    memset(charWidths, 0, charC);

    enforceUnicodeSheet = false;
    bidirectional = false;
    xPos = yPos = 0.0f;

    // Set up member variables
    m_cols = cols;
    m_rows = rows;
    m_charWidth = charWidth;
    m_charHeight = charHeight;
    m_textureLocation = textureLocation;

    // Build character map
    if (charMap != nullptr) {
        for (int i = 0; i < charC; i++) {
            m_charMap.insert(std::make_pair(charMap[i], i));
        }
    }

    random = new Random();

    // Load the image
    BufferedImage* img =
        textures->readImage(textureLocation->getTexture(), name);

    /* - 4J - TODO
    try {
    img = ImageIO.read(Textures.class.getResourceAsStream(name));
} catch (IOException e) {
    throw new RuntimeException(e);
}
    */

    int w = img->getWidth();
    int h = img->getHeight();
    std::vector<int> rawPixels(w * h);
    img->getRGB(0, 0, w, h, rawPixels, 0, w);

    for (int i = 0; i < charC; i++) {
        int xt = i % m_cols;
        int yt = i / m_cols;

        int x = 7;
        for (; x >= 0; x--) {
            int xPixel = xt * 8 + x;
            bool emptyColumn = true;
            for (int y = 0; y < 8 && emptyColumn; y++) {
                int yPixel = (yt * 8 + y) * w;
                bool emptyPixel = (rawPixels[xPixel + yPixel] >> 24) ==
                                  0;  // Check the alpha value
                if (!emptyPixel) emptyColumn = false;
            }
            if (!emptyColumn) {
                break;
            }
        }

        if (i == ' ') x = 4 - 2;
        charWidths[i] = x + 2;
    }

    delete img;

    // calculate colors
    for (int colorN = 0; colorN < 32; ++colorN) {
        int var10 = (colorN >> 3 & 1) * 85;
        int red = (colorN >> 2 & 1) * 170 + var10;
        int green = (colorN >> 1 & 1) * 170 + var10;
        int blue = (colorN >> 0 & 1) * 170 + var10;

        if (colorN == 6) {
            red += 85;
        }

        if (options->anaglyph3d) {
            int tmpRed = (red * 30 + green * 59 + blue * 11) / 100;
            int tmpGreen = (red * 30 + green * 70) / 100;
            int tmpBlue = (red * 30 + blue * 70) / 100;
            red = tmpRed;
            green = tmpGreen;
            blue = tmpBlue;
        }

        if (colorN >= 16) {
            red /= 4;
            green /= 4;
            blue /= 4;
        }

        colors[colorN] = (red & 255) << 16 | (green & 255) << 8 | (blue & 255);
    }
}

// 4J Stu - This dtor clashes with one in xui! We never delete these anyway so
// take it out for now. Can go back when we have got rid of XUI
Font::~Font() { delete[] charWidths; }

void Font::renderCharacter(char c) {
    float xOff = c % m_cols * m_charWidth;
    float yOff = c / m_cols * m_charWidth;

    float width = charWidths[c] - .01f;
    float height = m_charHeight - .01f;

    float fontWidth = m_cols * m_charWidth;
    float fontHeight = m_rows * m_charHeight;

    Tesselator* t = Tesselator::getInstance();
    // 4J Stu - Changed to a quad so that we can use within a command buffer
    t->begin();
    t->tex(xOff / fontWidth, (yOff + 7.99f) / fontHeight);
    t->vertex(xPos, yPos + height, 0.0f);

    t->tex((xOff + width) / fontWidth, (yOff + 7.99f) / fontHeight);
    t->vertex(xPos + width, yPos + height, 0.0f);

    t->tex((xOff + width) / fontWidth, yOff / fontHeight);
    t->vertex(xPos + width, yPos, 0.0f);

    t->tex(xOff / fontWidth, yOff / fontHeight);
    t->vertex(xPos, yPos, 0.0f);

    t->end();

    xPos += (float)charWidths[c];
}

void Font::drawShadow(const std::string& str, int x, int y, int color) {
    draw(str, x + 1, y + 1, color, true);
    draw(str, x, y, color, false);
}

void Font::drawShadowWordWrap(const std::string& str, int x, int y, int w,
                              int color, int h) {
    drawWordWrapInternal(str, x + 1, y + 1, w, color, true, h);
    drawWordWrapInternal(str, x, y, w, color, h);
}

void Font::draw(const std::string& str, int x, int y, int color) {
    draw(str, x, y, color, false);
}

std::string Font::reorderBidi(const std::string& str) {
    // 4J Not implemented
    return str;
}

void Font::draw(const std::string& str, bool dropShadow) {
    // Bind the texture
    textures->bindTexture(m_textureLocation);

    bool noise = false;
    std::string cleanStr = sanitize(str);

    for (int i = 0; i < (int)cleanStr.length(); ++i) {
        // Map character
        unsigned char c = cleanStr.at(i);

        // 4jcraft: this is a check for §. This was easy in UTF-16, since a
        // single widechar can fit §, but it's encoded as 0xA7 0xC2 in UTF-8, so
        // we need to check both characters.
        if (i + 2 < cleanStr.length() && c == 0xC2u &&
            (unsigned char)cleanStr[i + 1] == 0xA7u) {
            // 4J - following block was:
            // int colorN =
            // "0123456789abcdefk".indexOf(str.toLowerCase().charAt(i + 1));
            char ca = cleanStr[i + 1];
            int colorN = 16;
            if ((ca >= '0') && (ca <= '9'))
                colorN = ca - '0';
            else if ((ca >= 'a') && (ca <= 'f'))
                colorN = (ca - 'a') + 10;
            else if ((ca >= 'A') && (ca <= 'F'))
                colorN = (ca - 'A') + 10;

            if (colorN == 16) {
                noise = true;
            } else {
                noise = false;
                if (colorN < 0 || colorN > 15) colorN = 15;

                if (dropShadow) colorN += 16;

                int color = colors[colorN];
                glColor3f((color >> 16) / 255.0F, ((color >> 8) & 255) / 255.0F,
                          (color & 255) / 255.0F);
            }

            i += 1;
            continue;
        }

        // "noise" for crazy splash screen message
        if (noise) {
            int newc;
            do {
                newc = random->nextInt(
                    SharedConstants::acceptableLetters.length());
            } while (charWidths[c + 32] != charWidths[newc + 32]);
            c = newc;
        }

        renderCharacter(c);
    }
}

void Font::draw(const std::string& str, int x, int y, int color,
                bool dropShadow) {
    if (!str.empty()) {
        if ((color & 0xFC000000) == 0) color |= 0xFF000000;  // force alpha
        // if not set

        if (dropShadow)  // divide RGB by 4, preserve alpha
                         // 4jcraft changed -1 << 24 to the value of 1 (0xFF FF
                         // FF FF)
            color = (color & 0xfcfcfc) >> 2 | (color & (0xFFFFFFFF << 24));

        glColor4f((color >> 16 & 255) / 255.0F, (color >> 8 & 255) / 255.0F,
                  (color & 255) / 255.0F, (color >> 24 & 255) / 255.0F);

        xPos = x;
        yPos = y;
        draw(str, dropShadow);
    }
}

int Font::width(const std::string& str) {
    std::string cleanStr = sanitize(str);

    if (cleanStr == "") return 0;  // 4J - was nullptr comparison
    int len = 0;

    for (int i = 0; i < cleanStr.length(); ++i) {
        unsigned char c = cleanStr.at(i);

        // skip § (used for color codes)
        // 4jcraft: modified for UTF-8
        if (i + 2 < (int)cleanStr.length() && c == 0xC2 &&
            (unsigned char)cleanStr[i + 1] == 0xA7) {
            i += 2;
        } else {
            len += charWidths[c];
        }
    }

    return len;
}

std::string Font::sanitize(const std::string& str) {
    std::string sb = str;

    for (unsigned int i = 0; i < sb.length(); i++) {
        if (CharacterExists(sb[i])) {
            sb[i] = MapCharacter(sb[i]);
        } else {
            // If this character isn't supported, just show the first character
            // (empty square box character)
            sb[i] = 0;
        }
    }
    return sb;
}

int Font::MapCharacter(char c) {
    if (!m_charMap.empty()) {
        // Don't map space character
        return c == ' ' ? c : m_charMap[c];
    } else {
        return c;
    }
}

bool Font::CharacterExists(char c) {
    if (!m_charMap.empty()) {
        return m_charMap.find(c) != m_charMap.end();
    } else {
        return c >= 0 && c <= m_rows * m_cols;
    }
}

void Font::drawWordWrap(const std::string& string, int x, int y, int w, int col,
                        int h) {
    // if (bidirectional)
    //{
    //	string = reorderBidi(string);
    // }
    drawWordWrapInternal(string, x, y, w, col, h);
}

void Font::drawWordWrapInternal(const std::string& string, int x, int y, int w,
                                int col, int h) {
    drawWordWrapInternal(string, x, y, w, col, false, h);
}

void Font::drawWordWrap(const std::string& string, int x, int y, int w, int col,
                        bool darken, int h) {
    // if (bidirectional)
    //{
    //	string = reorderBidi(string);
    // }
    drawWordWrapInternal(string, x, y, w, col, darken, h);
}

void Font::drawWordWrapInternal(const std::string& string, int x, int y, int w,
                                int col, bool darken, int h) {
    std::vector<std::string> lines = stringSplit(string, '\n');
    if (lines.size() > 1) {
        auto itEnd = lines.end();
        for (auto it = lines.begin(); it != itEnd; it++) {
            // 4J Stu - Don't draw text that will be partially cutoff/overlap
            // something it shouldn't
            if ((y + this->wordWrapHeight(*it, w)) > h) break;
            drawWordWrapInternal(*it, x, y, w, col, h);
            y += this->wordWrapHeight(*it, w);
        }
        return;
    }
    std::vector<std::string> words = stringSplit(string, ' ');
    unsigned int pos = 0;
    while (pos < words.size()) {
        std::string line = words[pos++] + " ";
        while (pos < words.size() && width(line + words[pos]) < w) {
            line += words[pos++] + " ";
        }
        while (width(line) > w) {
            int l = 0;
            while (width(line.substr(0, l + 1)) <= w) {
                l++;
            }
            if (trimString(line.substr(0, l)).length() > 0) {
                draw(line.substr(0, l), x, y, col);
                y += 8;
            }
            line = line.substr(l);

            // 4J Stu - Don't draw text that will be partially cutoff/overlap
            // something it shouldn't
            if ((y + 8) > h) break;
        }
        // 4J Stu - Don't draw text that will be partially cutoff/overlap
        // something it shouldn't
        if (trimString(line).length() > 0 && !((y + 8) > h)) {
            draw(line, x, y, col);
            y += 8;
        }
    }
}

int Font::wordWrapHeight(const std::string& string, int w) {
    std::vector<std::string> lines = stringSplit(string, '\n');
    if (lines.size() > 1) {
        int h = 0;
        auto itEnd = lines.end();
        for (auto it = lines.begin(); it != itEnd; it++) {
            h += this->wordWrapHeight(*it, w);
        }
        return h;
    }
    std::vector<std::string> words = stringSplit(string, ' ');
    unsigned int pos = 0;
    int y = 0;
    while (pos < words.size()) {
        std::string line = words[pos++] + " ";
        while (pos < words.size() && width(line + words[pos]) < w) {
            line += words[pos++] + " ";
        }
        while (width(line) > w) {
            int l = 0;
            while (width(line.substr(0, l + 1)) <= w) {
                l++;
            }
            if (trimString(line.substr(0, l)).length() > 0) {
                y += 8;
            }
            line = line.substr(l);
        }
        if (trimString(line).length() > 0) {
            y += 8;
        }
    }
    if (y < 8) y += 8;
    return y;
}

void Font::setEnforceUnicodeSheet(bool enforceUnicodeSheet) {
    this->enforceUnicodeSheet = enforceUnicodeSheet;
}

void Font::setBidirectional(bool bidirectional) {
    this->bidirectional = bidirectional;
}

bool Font::AllCharactersValid(const std::string& str) {
    for (int i = 0; i < (int)str.length(); ++i) {
        unsigned char c = str.at(i);

        // skip § (used for color codes)
        // 4jcraft: modified for UTF-8
        if (i + 2 < (int)str.length() && c == 0xC2 &&
            (unsigned char)str[i + 1] == 0xA7) {
            i += 2;
            continue;
        }

        int index = SharedConstants::acceptableLetters.find(c);

        if ((c != ' ') && !(index > 0 && !enforceUnicodeSheet)) {
            return false;
        }
    }
    return true;
}

// Not in use
/*// 4J - this code is lifted from #if 0 section above, so that we can directly
create what would have gone in each of our 256 + 32 command buffers void
Font::renderFakeCB(IntBuffer *ib)
{
    Tesselator *t = Tesselator::getInstance();

        int i;

        for(unsigned int j = 0; j < ib->limit(); j++)
        {
                int cb = ib->get(j);

                if( cb < 256 )
                {
                        i = cb;
                        t->begin();
                        int ix = i % 16 * 8;
                        int iy = i / 16 * 8;
                        // float s = 7.99f;
                        float s = 7.99f;

                        float uo = (0.0f) / 128.0f;
                        float vo = (0.0f) / 128.0f;

                        t->vertexUV((float)(0), (float)( 0 + s), (float)( 0),
(float)( ix / 128.0f + uo), (float)( (iy + s) / 128.0f + vo));
                        t->vertexUV((float)(0 + s), (float)( 0 + s), (float)(
0), (float)( (ix + s) / 128.0f + uo), (float)( (iy + s) / 128.0f + vo));
                        t->vertexUV((float)(0 + s), (float)( 0), (float)( 0),
(float)( (ix + s) / 128.0f + uo), (float)( iy / 128.0f + vo));
                        t->vertexUV((float)(0), (float)( 0), (float)( 0),
(float)( ix / 128.0f + uo), (float)( iy / 128.0f + vo));
                        // target.colorBlit(texture, x + xo, y, color, ix, iy,
                // charWidths[chars[i]], 8);
                        t->end();

                        glTranslatef((float)charWidths[i], 0, 0);
                }
                else
                {
                        i = cb - 256;

                        int br = ((i >> 3) & 1) * 0x55;
                        int r = ((i >> 2) & 1) * 0xaa + br;
                        int g = ((i >> 1) & 1) * 0xaa + br;
                        int b = ((i >> 0) & 1) * 0xaa + br;
                        if (i == 6)
                        {
                                r += 0x55;
                        }
                        bool darken = i >= 16;

                        // color = r << 16 | g << 8 | b;
                        if (darken)
                        {
                                r /= 4;
                                g /= 4;
                                b /= 4;
                        }
                        glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
                }
        }
}

void Font::loadUnicodePage(int page)
{
        char fileName[25];
        //String fileName = String.format("/1_2_2/font/glyph_%02X.png", page);
        snprintf(fileName,25,"/1_2_2/font/glyph_%02X.png",page);
        BufferedImage *image = new BufferedImage(fileName);
        //try
        //{
        //	image =
ImageIO.read(Textures.class.getResourceAsStream(fileName.toWString()));
        //}
        //catch (IOException e)
        //{
        //	throw new RuntimeException(e);
        //}

        unicodeTexID[page] = textures->getTexture(image);
        lastBoundTexture = unicodeTexID[page];
}

void Font::renderUnicodeCharacter(char c)
{
        if (unicodeWidth[c] == 0)
        {
                // System.out.println("no-width char " + c);
                return;
        }

        int page = c / 256;

        if (unicodeTexID[page] == 0) loadUnicodePage(page);

        if (lastBoundTexture != unicodeTexID[page])
        {
                glBindTexture(GL_TEXTURE_2D, unicodeTexID[page]);
                lastBoundTexture = unicodeTexID[page];
        }

        // first column with non-trans pixels
        int firstLeft = unicodeWidth[c] >> 4;
        // last column with non-trans pixels
        int firstRight = unicodeWidth[c] & 0xF;

        float left = firstLeft;
        float right = firstRight + 1;

        float xOff = c % 16 * 16 + left;
        float yOff = (c & 0xFF) / 16 * 16;
        float width = right - left - .02f;

    Tesselator *t = Tesselator::getInstance();
        t->begin(GL_TRIANGLE_STRIP);
        t->tex(xOff / 256.0F, yOff / 256.0F);
        t->vertex(xPos, yPos, 0.0f);
        t->tex(xOff / 256.0F, (yOff + 15.98f) / 256.0F);
        t->vertex(xPos, yPos + 7.99f, 0.0f);
        t->tex((xOff + width) / 256.0F, yOff / 256.0F);
        t->vertex(xPos + width / 2, yPos, 0.0f);
        t->tex((xOff + width) / 256.0F, (yOff + 15.98f) / 256.0F);
        t->vertex(xPos + width / 2, yPos + 7.99f, 0.0f);
        t->end();

        xPos += (right - left) / 2 + 1;
}
*/
