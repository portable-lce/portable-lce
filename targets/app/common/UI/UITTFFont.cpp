#include "UITTFFont.h"

#include <assert.h>

#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "app/common/Iggy/include/rrCore.h"
#include "app/common/Game.h"
#include "platform/fs/fs.h"
#include "util/StringHelpers.h"

UITTFFont::UITTFFont(const std::string& name, const std::string& path,
                     S32 fallbackCharacter)
    : m_strFontName(name) {
    app.DebugPrintf("UITTFFont opening %s\n", path.c_str());
    pbData = nullptr;

    const std::size_t fileSize = PlatformFilesystem.fileSize(path);
    if (fileSize != 0) {
        pbData = new std::uint8_t[fileSize];
        auto result = PlatformFilesystem.readFile(path, pbData, fileSize);
        if (result.status != IPlatformFilesystem::ReadStatus::Ok) {
            app.DebugPrintf("Failed to open TTF file\n");
            delete[] pbData;
            pbData = nullptr;
            app.FatalLoadError();
        }

        IggyFontInstallTruetypeUTF8((void*)pbData, IGGY_TTC_INDEX_none,
                                    m_strFontName.c_str(), -1,
                                    IGGY_FONTFLAG_none);

        IggyFontInstallTruetypeFallbackCodepointUTF8(
            "Mojangles_TTF", -1, IGGY_FONTFLAG_none, fallbackCharacter);

        // 4J Stu - These are so we can use the default flash controls
        IggyFontInstallTruetypeUTF8((void*)pbData, IGGY_TTC_INDEX_none,
                                    "Times New Roman", -1, IGGY_FONTFLAG_none);
        IggyFontInstallTruetypeUTF8((void*)pbData, IGGY_TTC_INDEX_none, "Arial",
                                    -1, IGGY_FONTFLAG_none);
    } else {
        app.DebugPrintf("Failed to open TTF file\n");
        assert(false);
    }
}

UITTFFont::~UITTFFont() {}

std::string UITTFFont::getFontName() { return m_strFontName; }