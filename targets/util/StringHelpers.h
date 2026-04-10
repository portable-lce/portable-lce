#pragma once

#include <sstream>
#include <string>
#include <vector>

std::string toLower(const std::string& a);
std::string trimString(const std::string& a);
std::string replaceAll(const std::string& in, const std::string& replace,
                       const std::string& with);

bool equalsIgnoreCase(const std::string& a, const std::string& b);

template <class T>
std::string toWString(T t) {
    std::ostringstream oss;
    oss << std::dec << t;
    return oss.str();
}
template <class T>
T fromWString(const std::string& s) {
    std::istringstream stream(s);
    T t;
    stream >> t;
    return t;
}
template <class T>
T fromHexWString(const std::string& s) {
    std::istringstream stream(s);
    T t;
    stream >> std::hex >> t;
    return t;
}

std::wstring convStringToWstring(const std::string& converting);
std::wstring u16string_to_wstring(const std::u16string& converting);
std::u16string wstring_to_u16string(const std::wstring& converting);
std::u16string string_to_u16string(const std::string& converting);
std::u8string wstring_to_u8string(const std::wstring& converting);

std::vector<std::string>& stringSplit(const std::string& s, char delim,
                                      std::vector<std::string>& elems);
std::vector<std::string> stringSplit(const std::string& s, char delim);

void stripWhitespaceForHtml(std::string& string, bool bRemoveNewline = true);
std::string escapeXML(const std::string& in);
std::string parseXMLSpecials(const std::string& in);
