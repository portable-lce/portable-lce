#include "DLCAudioFile.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>

#include "DLCManager.h"
#include "app/common/DLC/DLCFile.h"
#include "app/common/Game.h"
#include "platform/XboxStubs.h"
#include "platform/renderer/renderer.h"
#include "platform/storage/storage.h"
#include "util/StringHelpers.h"

#if defined(_WINDOWS64)
#include "app/windows/XML/ATGXmlParser.h"
#include "app/windows/XML/xmlFilesCallback.h"
#endif

namespace {
constexpr std::size_t AUDIO_DLC_WCHAR_BIN_SIZE = 2;

template <typename T>
T ReadAudioDlcValue(const std::uint8_t* data, unsigned int offset = 0) {
    T value;
    std::memcpy(&value, data + offset, sizeof(value));
    return value;
}

template <typename T>
void ReadAudioDlcStruct(T* out, const std::uint8_t* data,
                        unsigned int offset = 0) {
    std::memcpy(out, data + offset, sizeof(*out));
}

inline unsigned int AudioParamAdvance(unsigned int wcharCount) {
    return static_cast<unsigned int>(sizeof(IPlatformStorage::DLC_FILE_PARAM) +
                                     wcharCount * AUDIO_DLC_WCHAR_BIN_SIZE);
}

inline unsigned int AudioDetailAdvance(unsigned int wcharCount) {
    return static_cast<unsigned int>(
        sizeof(IPlatformStorage::DLC_FILE_DETAILS) +
        wcharCount * AUDIO_DLC_WCHAR_BIN_SIZE);
}

inline std::string ReadAudioParamString(const std::uint8_t* data,
                                        unsigned int offset) {
    return dlc_read_wstring(
        data + offset + offsetof(IPlatformStorage::DLC_FILE_PARAM, wchData));
}
}  // namespace

DLCAudioFile::DLCAudioFile(const std::string& path)
    : DLCFile(DLCManager::e_DLCType_Audio, path) {
    m_pbData = nullptr;
    m_dataBytes = 0;
}

void DLCAudioFile::addData(std::uint8_t* pbData, std::uint32_t dataBytes) {
    m_pbData = pbData;
    m_dataBytes = dataBytes;

    processDLCDataFile(pbData, dataBytes);
}

std::uint8_t* DLCAudioFile::getData(std::uint32_t& dataBytes) {
    dataBytes = m_dataBytes;
    return m_pbData;
}

const char* DLCAudioFile::wchTypeNamesA[] = {
    "CUENAME",
    "CREDIT",
};

DLCAudioFile::EAudioParameterType DLCAudioFile::getParameterType(
    const std::string& paramName) {
    EAudioParameterType type = e_AudioParamType_Invalid;

    for (int i = 0; i < e_AudioParamType_Max; ++i) {
        if (paramName.compare(wchTypeNamesA[i]) == 0) {
            type = (EAudioParameterType)i;
            break;
        }
    }

    return type;
}

void DLCAudioFile::addParameter(EAudioType type, EAudioParameterType ptype,
                                const std::string& value) {
    switch (ptype) {
        case e_AudioParamType_Credit:  // If this parameter exists, then mark
                                       // this as free
            // add it to the DLC credits list

            // we'll need to justify this text since we don't have a lot of room
            // for lines of credits
            {
                // don't look for duplicate in the music credits

                // if(app.AlreadySeenCreditText(value)) break;

                int maximumChars = 55;

                bool bIsSDMode = !PlatformRenderer.IsHiDef() &&
                                 !PlatformRenderer.IsWidescreen();

                if (bIsSDMode) {
                    maximumChars = 45;
                }

                switch (XGetLanguage()) {
                    case XC_LANGUAGE_JAPANESE:
                    case XC_LANGUAGE_TCHINESE:
                    case XC_LANGUAGE_KOREAN:
                        maximumChars = 35;
                        break;
                }
                std::string creditValue = value;
                while (creditValue.length() > maximumChars) {
                    unsigned int i = 1;
                    while (i < creditValue.length() &&
                           (i + 1) <= maximumChars) {
                        i++;
                    }
                    int iLast = (int)creditValue.find_last_of(" ", i);
                    switch (XGetLanguage()) {
                        case XC_LANGUAGE_JAPANESE:
                        case XC_LANGUAGE_TCHINESE:
                        case XC_LANGUAGE_KOREAN:
                            iLast = maximumChars;
                            break;
                        default:
                            iLast = (int)creditValue.find_last_of(" ", i);
                            break;
                    }

                    // if a space was found, include the space on this line
                    if (iLast != i) {
                        iLast++;
                    }

                    app.AddCreditText((creditValue.substr(0, iLast)).c_str());
                    creditValue = creditValue.substr(iLast);
                }
                app.AddCreditText(creditValue.c_str());
            }
            break;
        case e_AudioParamType_Cuename:
            m_parameters[type].push_back(value);
            // m_parameters[(int)type] = value;
            break;
        default:
            break;
    }
}

bool DLCAudioFile::processDLCDataFile(std::uint8_t* pbData,
                                      std::uint32_t dataLength) {
    std::unordered_map<int, EAudioParameterType> parameterMapping;
    unsigned int uiCurrentByte = 0;

    // File format defined in the AudioPacker
    // File format: Version 1

    unsigned int uiVersion =
        ReadAudioDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);

    if (uiVersion < CURRENT_AUDIO_VERSION_NUM) {
        if (pbData != nullptr) delete[] pbData;
        app.DebugPrintf("DLC version of %d is too old to be read\n", uiVersion);
        return false;
    }

    unsigned int uiParameterTypeCount =
        ReadAudioDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);
    IPlatformStorage::DLC_FILE_PARAM paramBuf;
    ReadAudioDlcStruct(&paramBuf, pbData, uiCurrentByte);

    for (unsigned int i = 0; i < uiParameterTypeCount; i++) {
        // Map DLC strings to application strings, then store the DLC index
        // mapping to application index
        std::string parameterName = ReadAudioParamString(pbData, uiCurrentByte);
        EAudioParameterType type = getParameterType(parameterName);
        if (type != e_AudioParamType_Invalid) {
            parameterMapping[paramBuf.dwType] = type;
        }
        uiCurrentByte += AudioParamAdvance(paramBuf.dwWchCount);
        ReadAudioDlcStruct(&paramBuf, pbData, uiCurrentByte);
    }
    unsigned int uiFileCount =
        ReadAudioDlcValue<unsigned int>(pbData, uiCurrentByte);
    uiCurrentByte += sizeof(int);
    IPlatformStorage::DLC_FILE_DETAILS fileBuf;
    ReadAudioDlcStruct(&fileBuf, pbData, uiCurrentByte);

    unsigned int tempByteOffset = uiCurrentByte;
    for (unsigned int i = 0; i < uiFileCount; i++) {
        tempByteOffset += AudioDetailAdvance(fileBuf.dwWchCount);
        ReadAudioDlcStruct(&fileBuf, pbData, tempByteOffset);
    }
    std::uint8_t* pbTemp = &pbData[tempByteOffset];
    ReadAudioDlcStruct(&fileBuf, pbData, uiCurrentByte);

    for (unsigned int i = 0; i < uiFileCount; i++) {
        EAudioType type = (EAudioType)fileBuf.dwType;
        // Params
        unsigned int uiParameterCount = ReadAudioDlcValue<unsigned int>(pbTemp);
        pbTemp += sizeof(int);
        ReadAudioDlcStruct(&paramBuf, pbTemp);
        for (unsigned int j = 0; j < uiParameterCount; j++) {
            // EAudioParameterType paramType = e_AudioParamType_Invalid;

            auto it = parameterMapping.find(paramBuf.dwType);

            if (it != parameterMapping.end()) {
                addParameter(type, (EAudioParameterType)paramBuf.dwType,
                             ReadAudioParamString(pbTemp, 0));
            }
            pbTemp += AudioParamAdvance(paramBuf.dwWchCount);
            ReadAudioDlcStruct(&paramBuf, pbTemp);
        }
        // Move the pointer to the start of the next files data;
        pbTemp += fileBuf.uiFileSize;
        uiCurrentByte += AudioDetailAdvance(fileBuf.dwWchCount);

        ReadAudioDlcStruct(&fileBuf, pbData, uiCurrentByte);
    }

    return true;
}

int DLCAudioFile::GetCountofType(DLCAudioFile::EAudioType eType) {
    return m_parameters[eType].size();
}

std::string& DLCAudioFile::GetSoundName(int iIndex) {
    int iWorldType = e_AudioType_Overworld;
    while (iIndex >= m_parameters[iWorldType].size()) {
        iIndex -= m_parameters[iWorldType].size();
        iWorldType++;
    }
    return m_parameters[iWorldType].at(iIndex);
}
