#include "minecraft/locale/StringTable.h"

#include <ranges>
#include <utility>

#include "java/InputOutputStream/ByteArrayInputStream.h"
#include "java/InputOutputStream/DataInputStream.h"
#include "minecraft/util/Log.h"

StringTable::StringTable(void) {}

// Load string table from a binary blob, filling out with the current
// localisation data only. The caller passes the locale list it wants
// matched (typically obtained from the LocalizationManager) so that
// StringTable does not have to know about app-side singletons.
StringTable::StringTable(std::span<const std::uint8_t> data,
                         std::span<const std::string> locales)
    : src(data.begin(), data.end()) {
    ProcessStringTableData(locales);
}

void StringTable::ReloadStringTable(std::span<const std::string> locales) {
    m_stringsMap.clear();
    m_stringsVec.clear();

    ProcessStringTableData(locales);
}

void StringTable::ProcessStringTableData(std::span<const std::string> locales) {
    ByteArrayInputStream bais(src);
    DataInputStream dis(&bais);

    int versionNumber = dis.readInt();
    int languagesCount = dis.readInt();

    std::vector<std::pair<std::string, int> > langSizeMap;
    for (int i = 0; i < languagesCount; ++i) {
        std::string langId = dis.readUTF();
        int langSize = dis.readInt();

        langSizeMap.push_back(
            std::vector<std::pair<std::string, int> >::value_type(langId,
                                                                  langSize));
    }

    bool foundLang = false;
    int64_t bytesToSkip = 0;
    int dataSize = 0;

    //
    for (auto it_locales = locales.begin();
         it_locales != locales.end() && (!foundLang); it_locales++) {
        bytesToSkip = 0;

        for (auto it = langSizeMap.begin(); it != langSizeMap.end(); ++it) {
            if (it->first.compare(*it_locales) == 0) {
                Log::info("StringTable:: Found language '%s'.\n",
                          it_locales->c_str());
                dataSize = it->second;
                foundLang = true;
                break;
            }

            bytesToSkip += it->second;
        }

        if (!foundLang)
            Log::info("StringTable:: Can't find language '%s'.\n",
                      it_locales->c_str());
    }

    if (foundLang) {
        dis.skip(bytesToSkip);

        std::vector<uint8_t> langData(dataSize);
        dis.read(langData);

        dis.close();

        ByteArrayInputStream bais2(langData);
        DataInputStream dis2(&bais2);

        // Read the language file for the selected language
        int langVersion = dis2.readInt();

        isStatic = false;     // 4J-JEV: Versions 1 and up could use
        if (langVersion > 0)  // integers rather than std::wstrings as keys.
            isStatic = dis2.readBoolean();

        std::string langId = dis2.readUTF();
        int totalStrings = dis2.readInt();

        Log::info("IsStatic=%d totalStrings = %d\n", isStatic ? 1 : 0,
                  totalStrings);

        if (!isStatic) {
            for (int i = 0; i < totalStrings; ++i) {
                std::string stringId = dis2.readUTF();
                std::string stringValue = dis2.readUTF();

                m_stringsMap.insert(
                    std::unordered_map<std::string, std::string>::value_type(
                        stringId, stringValue));
            }
        } else {
            for (int i = 0; i < totalStrings; ++i)
                m_stringsVec.push_back(dis2.readUTF());
        }
        dis2.close();

        // We can't delete this data in the dtor, so clear the reference
        bais2.reset();
    } else {
        Log::info("Failed to get language\n");
#ifdef _DEBUG
        assert(0);
#endif

        isStatic = false;
    }

    // We can't delete this data in the dtor, so clear the reference
    bais.reset();
}

StringTable::~StringTable(void) {
    // delete src.data(); TODO 4J-JEV: ?
}

void StringTable::getData(std::uint8_t** ppData, unsigned int* pSize) {
    *ppData = src.data();
    *pSize = src.size();
}

const char* StringTable::getString(const std::string& id) {
#ifndef _CONTENT_PACKAGE
    if (isStatic) {
        assert(0);
        return "";
    }
#endif

    auto it = m_stringsMap.find(id);

    if (it != m_stringsMap.end()) {
        return it->second.c_str();
    } else {
        return "";
    }
}

const char* StringTable::getString(int id) {
#ifndef _CONTENT_PACKAGE
    if (!isStatic) {
        assert(0);
        return "";
    }
#endif

    if (id < m_stringsVec.size()) {
        const char* pwchString = m_stringsVec.at(id).c_str();
        return pwchString;
    } else
        return "";
}
