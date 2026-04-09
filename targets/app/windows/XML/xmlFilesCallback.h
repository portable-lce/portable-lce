
#pragma once
#if !defined(XMLMOJANGCALLBACK_H)
#define XMLMOJANGCALLBACK_H
// xml reading

using namespace ATG;

class xmlMojangCallback : public ATG::ISAXCallback {
public:
    virtual int32_t StartDocument() { return 0; };
    virtual int32_t EndDocument() { return 0; };

    virtual int32_t ElementBegin(const char* strName, uint32_t NameLen,
                                 const XMLAttribute* pAttributes,
                                 uint32_t NumAttributes) {
        char wTemp[35] = "";
        char wAttName[32] = "";
        char wNameXUID[32] = "";
        char wNameSkin[32] = "";
        char wNameCloak[32] = "";
        PlayerUID xuid = 0LL;

        if (NameLen > 31)
            return 1;
        else
            strncpy(wAttName, strName, NameLen);

        if (_wcsicmp(wAttName, "root") == 0) {
            return 0;
        } else if (_wcsicmp(wAttName, "data") == 0) {
            for (uint32_t i = 0; i < NumAttributes; i++) {
                wcsncpy_s(wAttName, pAttributes[i].strName,
                          pAttributes[i].NameLen);
                if (_wcsicmp(wAttName, "name") == 0) {
                    if (pAttributes[i].ValueLen <= 32)
                        wcsncpy_s(wNameXUID, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                } else if (_wcsicmp(wAttName, "xuid") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        memset(wTemp, 0, sizeof(char) * 35);
                        wcsncpy_s(wTemp, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                        xuid = _wcstoui64(wTemp, nullptr, 10);
                    }
                } else if (_wcsicmp(wAttName, "cape") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wNameCloak, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                } else if (_wcsicmp(wAttName, "skin") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wNameSkin, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                }
            }

            // if the xuid hasn't been defined, then we can't use the data
            if (xuid != 0LL) {
                return Game::RegisterMojangData(wNameXUID, xuid, wNameSkin,
                                                wNameCloak);
            } else
                return 1;
        } else {
            return 1;
        }
    };

    virtual int32_t ElementContent(const char* strData, uint32_t DataLen,
                                   bool More) {
        return 0;
    };

    virtual int32_t ElementEnd(const char* strName, uint32_t NameLen) {
        return 0;
    };

    virtual int32_t CDATABegin() { return 0; };

    virtual int32_t CDATAData(const char* strCDATA, uint32_t CDATALen,
                              bool bMore) {
        return 0;
    };

    virtual int32_t CDATAEnd() { return 0; };

    virtual void Error(int32_t hError, const char* strMessage) {
        app.DebugPrintf("Error when Parsing xuids.XML\n");
    };
};

class xmlConfigCallback : public ATG::ISAXCallback {
public:
    virtual int32_t StartDocument() { return 0; };
    virtual int32_t EndDocument() { return 0; };

    virtual int32_t ElementBegin(const char* strName, uint32_t NameLen,
                                 const XMLAttribute* pAttributes,
                                 uint32_t NumAttributes) {
        char wTemp[35] = "";
        char wType[32] = "";
        char wAttName[32] = "";
        char wValue[32] = "";
        int iValue = -1;

        if (NameLen > 31)
            return 1;
        else
            wcsncpy_s(wAttName, strName, NameLen);

        if (_wcsicmp(wAttName, "root") == 0) {
            return 0;
        } else if (_wcsicmp(wAttName, "data") == 0) {
            for (uint32_t i = 0; i < NumAttributes; i++) {
                wcsncpy_s(wAttName, pAttributes[i].strName,
                          pAttributes[i].NameLen);
                if (_wcsicmp(wAttName, "Type") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wType, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                } else if (_wcsicmp(wAttName, "Value") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wValue, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);

                        iValue = strtol(wValue, nullptr, 10);
                    }
                }
            }

            // if the xuid hasn't been defined, then we can't use the data
            if (iValue != -1) {
#if defined(_DEBUG)
                printf("Type - %s, Value - %d, ", wType, iValue);
#endif

                return Game::RegisterConfigValues(wType, iValue);
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    }

    virtual int32_t ElementContent(const char* strData, uint32_t DataLen,
                                   bool More) {
        return 0;
    };

    virtual int32_t ElementEnd(const char* strName, uint32_t NameLen) {
        return 0;
    };

    virtual int32_t CDATABegin() { return 0; };

    virtual int32_t CDATAData(const char* strCDATA, uint32_t CDATALen,
                              bool bMore) {
        return 0;
    };

    virtual int32_t CDATAEnd() { return 0; };

    virtual void Error(int32_t hError, const char* strMessage) {
        app.DebugPrintf("Error when Parsing xuids.XML\n");
    };
};

class xmlDLCInfoCallback : public ATG::ISAXCallback {
public:
    virtual int32_t StartDocument() { return 0; };
    virtual int32_t EndDocument() { return 0; };

    virtual int32_t ElementBegin(const char* strName, uint32_t NameLen,
                                 const XMLAttribute* pAttributes,
                                 uint32_t NumAttributes) {
        char wTemp[35] = "";
        char wAttName[32] = "";
        char wNameBanner[32] = "";
        char wDataFile[32] = "";
        char wType[32] = "";
        char wFirstSkin[32] = "";
        char wConfig[32] = "";
        uint64_t ullFull = 0ll;
        uint64_t ullTrial = 0ll;
        unsigned int uiSortIndex = 0L;
        int iGender = 0;
        int iConfig = 0;

        if (NameLen > 31)
            return 1;
        else
            wcsncpy_s(wAttName, strName, NameLen);

        if (_wcsicmp(wAttName, "root") == 0) {
            return 0;
        } else if (_wcsicmp(wAttName, "data") == 0) {
            for (uint32_t i = 0; i < NumAttributes; i++) {
                wcsncpy_s(wAttName, pAttributes[i].strName,
                          pAttributes[i].NameLen);
                if (_wcsicmp(wAttName, "SortIndex") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        memset(wTemp, 0, sizeof(char) * 35);
                        wcsncpy_s(wTemp, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                        uiSortIndex = wcstoul(wTemp, nullptr, 16);
                    }
                } else if (_wcsicmp(wAttName, "Banner") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wNameBanner, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                } else if (_wcsicmp(wAttName, "Full") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        memset(wTemp, 0, sizeof(char) * 35);
                        wcsncpy_s(wTemp, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                        ullFull = _wcstoui64(wTemp, nullptr, 16);
                    }
                } else if (_wcsicmp(wAttName, "Trial") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        memset(wTemp, 0, sizeof(char) * 35);
                        wcsncpy_s(wTemp, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                        ullTrial = _wcstoui64(wTemp, nullptr, 16);
                    }
                } else if (_wcsicmp(wAttName, "FirstSkin") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wFirstSkin, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                } else if (_wcsicmp(wAttName, "Type") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wType, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                } else if (_wcsicmp(wAttName, "Gender") == 0) {
                    if (_wcsicmp(wAttName, "Male") == 0) {
                        iGender = 1;
                    } else if (_wcsicmp(wAttName, "Female") == 0) {
                        iGender = 2;
                    } else {
                        iGender = 0;
                    }
                } else if (_wcsicmp(wAttName, "Config") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wConfig, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);

                        iConfig = strtol(wConfig, nullptr, 10);
                    }
                } else if (_wcsicmp(wAttName, "DataFile") == 0) {
                    if (pAttributes[i].ValueLen <= 32) {
                        wcsncpy_s(wDataFile, pAttributes[i].strValue,
                                  pAttributes[i].ValueLen);
                    }
                }
            }

            // if the xuid hasn't been defined, then we can't use the data
            if (ullFull != 0LL) {
#if defined(_DEBUG)
                printf("Type - %s, Name - %s, ", wType, wNameBanner);
#endif
                app.DebugPrintf("Full = %lld, Trial %lld\n", ullFull, ullTrial);

                return Game::RegisterDLCData(wType, wNameBanner, iGender,
                                             ullFull, ullTrial, wFirstSkin,
                                             uiSortIndex, iConfig, wDataFile);
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    };

    virtual int32_t ElementContent(const char* strData, uint32_t DataLen,
                                   bool More) {
        return 0;
    };

    virtual int32_t ElementEnd(const char* strName, uint32_t NameLen) {
        return 0;
    };

    virtual int32_t CDATABegin() { return 0; };

    virtual int32_t CDATAData(const char* strCDATA, uint32_t CDATALen,
                              bool bMore) {
        return 0;
    };

    virtual int32_t CDATAEnd() { return 0; };

    virtual void Error(int32_t hError, const char* strMessage) {
        app.DebugPrintf("Error when Parsing DLC.XML\n");
    };
};

#endif
