#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cwctype>
#include <sstream>
#include <string>
#include <vector>

#include "simdutf.h"

// fuck you, MSVC.
static_assert((unsigned char)"ඞ"[0] == 0xE0 && (unsigned char)"ඞ"[1] == 0xB6 &&
                  (unsigned char)"ඞ"[2] == 0x9E,
              "Execution charset is not UTF-8. Compile with "
              "-fexec-charset=UTF-8 or /utf-8, or use a compiler written by a "
              "human being with a functional brain.");

std::string toLower(const std::string& a) {
    std::string out = std::string(a);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

// 4jcraft TODO: this intentionally returns the original string (not empty)
// for whitespace-only input. Callers in animation file parsing
// (AbstractTexturePack::getAnimationString) depend on this behavior -
// returning empty here breaks clock/compass texture frame loading.
std::string trimString(const std::string& a) {
    std::string b;
    int start = (int)a.find_first_not_of(" \t\n\r");
    int end = (int)a.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) start = 0;
    if (end == std::string::npos) end = (int)a.size() - 1;
    b = a.substr(start, (end - start) + 1);
    return b;
}

std::string replaceAll(const std::string& in, const std::string& replace,
                       const std::string& with) {
    std::string out = in;
    size_t pos = 0;
    while ((pos = out.find(replace, pos)) != std::string::npos) {
        out.replace(pos, replace.length(), with);
        pos++;
    }
    return out;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    bool out;
    std::string c = toLower(a);
    std::string d = toLower(b);
    out = c.compare(d) == 0;
    return out;
}

std::wstring convStringToWstring(const std::string& converting) {
    std::wstring converted(converting.length(), L' ');
    copy(converting.begin(), converting.end(), converted.begin());
    return converted;
}

std::wstring u16string_to_wstring(const std::u16string& converting) {
    if constexpr (sizeof(wchar_t) == 2) {
        // on Windows, wchar_t is UTF-16 so we can get away with just a type
        // transmutation
        return std::wstring(reinterpret_cast<const wchar_t*>(converting.data()),
                            converting.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        // POSIX has wchar_t as UTF-32 instead so simdutf time :>>>
        if (converting.empty()) return {};

        std::wstring result(simdutf::utf32_length_from_utf16(converting.data(),
                                                             converting.size()),
                            L'\0');
        std::size_t convertedLength = simdutf::convert_utf16_to_utf32(
            converting.data(), converting.size(),
            reinterpret_cast<char32_t*>(result.data()));
        result.resize(convertedLength);

        return result;
    } else {
        static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4,
                      "Here's a nickel, Kid. Go buy yourself a real computer.");
    }
}

std::u16string wstring_to_u16string(const std::wstring& converting) {
    if constexpr (sizeof(wchar_t) == 2) {
        // Windows, UTF-16
        return std::u16string(
            reinterpret_cast<const char16_t*>(converting.data()),
            converting.size());
    } else if constexpr (sizeof(wchar_t) == 4) {
        // POSIX, UTF-32
        if (converting.empty()) return {};

        auto data32 = reinterpret_cast<const char32_t*>(converting.data());
        auto len32 = converting.size();

        std::u16string result(simdutf::utf16_length_from_utf32(data32, len32),
                              u'\0');
        auto len =
            simdutf::convert_utf32_to_utf16(data32, len32, result.data());
        result.resize(len);

        return result;
    } else {
        static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4,
                      "Here's a nickel, Kid. Go buy yourself a real computer.");
    }
}

std::u8string wstring_to_u8string(const std::wstring& converting) {
    if (converting.empty()) return {};

    if constexpr (sizeof(wchar_t) == 2) {
        auto data16 = reinterpret_cast<const char16_t*>(converting.data());
        auto len16 = converting.size();

        std::u8string result(simdutf::utf8_length_from_utf16le(data16, len16),
                             u'\0');
        auto len = simdutf::convert_utf16_to_utf8(
            data16, len16, reinterpret_cast<char*>(result.data()));
        result.resize(len);

        return result;
    } else if constexpr (sizeof(wchar_t) == 4) {
        auto data32 = reinterpret_cast<const char32_t*>(converting.data());
        auto len32 = converting.size();

        std::u8string result(simdutf::utf8_length_from_utf32(data32, len32),
                             u'\0');
        auto len = simdutf::convert_utf32_to_utf8(
            data32, len32, reinterpret_cast<char*>(result.data()));
        result.resize(len);

        return result;
    } else {
        static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4,
                      "Here's a nickel, Kid. Go buy yourself a real computer.");
    }
}

std::u16string string_to_u16string(const std::string& converting) {
    if (converting.empty()) return {};

    std::u16string result(
        simdutf::utf16_length_from_utf8(converting.data(), converting.size()),
        u'\0');
    std::size_t convertedLength = simdutf::convert_utf8_to_utf16(
        converting.data(), converting.size(), result.data());
    result.resize(convertedLength);

    return result;
}

std::vector<std::string>& stringSplit(const std::string& s, char delim,
                                      std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> stringSplit(const std::string& s, char delim) {
    std::vector<std::string> elems;
    return stringSplit(s, delim, elems);
}

bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }

void stripWhitespaceForHtml(std::string& string, bool bRemoveNewline) {
    // Strip newline chars
    if (bRemoveNewline) {
        string.erase(std::remove(string.begin(), string.end(), '\n'),
                     string.end());
        string.erase(std::remove(string.begin(), string.end(), '\r'),
                     string.end());
    }

    string.erase(std::remove(string.begin(), string.end(), '\t'), string.end());

    // Strip duplicate spaces
    string.erase(std::unique(string.begin(), string.end(), BothAreSpaces),
                 string.end());

    string = trimString(string);
}

std::string escapeXML(const std::string& in) {
    std::string out = in;
    out = replaceAll(out, "&", "&amp;");
    // out = replaceAll(out, "\"", "&quot;");
    // out = replaceAll(out, "'", "&apos;");
    out = replaceAll(out, "<", "&lt;");
    out = replaceAll(out, ">", "&gt;");
    return out;
}

std::string parseXMLSpecials(const std::string& in) {
    std::string out = in;
    out = replaceAll(out, "&amp;", "&");
    // out = replaceAll(out, "\"", "&quot;");
    // out = replaceAll(out, "'", "&apos;");
    out = replaceAll(out, "&lt;", "<");
    out = replaceAll(out, "&gt;", ">");
    return out;
}
