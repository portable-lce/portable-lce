// 4J-PB -
// The ATG Framework is a common set of C++ class libraries that is used by the
// samples in the XDK, and was developed by the Advanced Technology Group (ATG).
// The ATG Framework offers a clean and consistent format for the samples. These
// classes define functions used by all the samples. The ATG Framework together
// with the samples demonstrates best practices and innovative techniques for
// Xbox 360. There are many useful sections of code in the samples. You are
// encouraged to incorporate this code into your titles.

//-------------------------------------------------------------------------------------
//  AtgXmlParser.h
//
//  XMLParser and SAX interface declaration
//
//  Xbox Advanced Technology Group
//  Copyright (C) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef ATGXMLPARSER_H
#define ATGXMLPARSER_H

namespace ATG {

//-----------------------------------------------------------------------------
// error returns from XMLParse
//-----------------------------------------------------------------------------
#define _ATGFAC 0x61B
#define E_COULD_NOT_OPEN_FILE MAKE_HRESULT(1, _ATGFAC, 0x0001)
#define E_INVALID_XML_SYNTAX MAKE_HRESULT(1, _ATGFAC, 0x0002)

const uint32_t XML_MAX_ATTRIBUTES_PER_ELEMENT = 32;
const uint32_t XML_MAX_NAME_LENGTH = 128;
const uint32_t XML_READ_BUFFER_SIZE = 2048;
const uint32_t XML_WRITE_BUFFER_SIZE = 2048;

// No tag can be longer than XML_WRITE_BUFFER_SIZE - an error will be returned
// if it is

//-------------------------------------------------------------------------------------
struct XMLAttribute {
    char* strName;
    uint32_t NameLen;
    char* strValue;
    uint32_t ValueLen;
};

//-------------------------------------------------------------------------------------
class ISAXCallback {
    friend class XMLParser;

public:
    ISAXCallback(){};
    virtual ~ISAXCallback(){};

    virtual int32_t StartDocument() = 0;
    virtual int32_t EndDocument() = 0;

    virtual int32_t ElementBegin(const char* strName, uint32_t NameLen,
                                 const XMLAttribute* pAttributes,
                                 uint32_t NumAttributes) = 0;
    virtual int32_t ElementContent(const char* strData, uint32_t DataLen,
                                   bool More) = 0;
    virtual int32_t ElementEnd(const char* strName, uint32_t NameLen) = 0;

    virtual int32_t CDATABegin() = 0;
    virtual int32_t CDATAData(const char* strCDATA, uint32_t CDATALen,
                              bool bMore) = 0;
    virtual int32_t CDATAEnd() = 0;

    virtual void Error(int32_t hError, const char* strMessage) = 0;

    virtual void SetParseProgress(uint32_t dwProgress) {}

    const char* GetFilename() { return m_strFilename; }
    uint32_t GetLineNumber() { return m_LineNum; }
    uint32_t GetLinePosition() { return m_LinePos; }

private:
    const char* m_strFilename;
    uint32_t m_LineNum;
    uint32_t m_LinePos;
};

//-------------------------------------------------------------------------------------
class XMLParser {
public:
    XMLParser();
    ~XMLParser();

    //      Register an interface inheiriting from ISAXCallback
    void RegisterSAXCallbackInterface(ISAXCallback* pISAXCallback);

    //      Get the registered interface
    ISAXCallback* GetSAXCallbackInterface();

    //      ParseXMLFile returns one of the following:
    //         E_COULD_NOT_OPEN_FILE - couldn't open the file
    //         E_INVALID_XML_SYNTAX - bad XML syntax according to this parser
    //         E_NOINTERFACE - RegisterSAXCallbackInterface not called
    //         E_ABORT - callback returned a fail code
    //         S_OK - file parsed and completed

    int32_t ParseXMLFile(const char* strFilename);

    //      Parses from a buffer- if you pass a char buffer (and cast it), it
    //      will
    //         correctly detect it and use unicode instead.  Return codes are
    //         the same as for ParseXMLFile

    int32_t ParseXMLBuffer(const char* strBuffer, uint32_t uBufferSize);

private:
    int32_t MainParseLoop();

    int32_t AdvanceCharacter(bool bOkToFail = false);
    void SkipNextAdvance();

    int32_t ConsumeSpace();
    int32_t ConvertEscape();
    int32_t AdvanceElement();
    int32_t AdvanceName();
    int32_t AdvanceAttrVal();
    int32_t AdvanceCDATA();
    int32_t AdvanceComment();

    void FillBuffer();

#ifdef _Printf_format_string_  // VC++ 2008 and later support this annotation
    void Error(int32_t hRet,
               _In_z_ _Printf_format_string_ const char* strFormat, ...);
#else
    void Error(int32_t hRet, const char* strFormat, ...);
#endif

    ISAXCallback* m_pISAXCallback;

    void* m_hFile;
    const char* m_pInXMLBuffer;
    uint32_t m_uInXMLBufferCharsLeft;
    uint32_t m_dwCharsTotal;
    uint32_t m_dwCharsConsumed;

    uint8_t m_pReadBuf[XML_READ_BUFFER_SIZE + 2];  // room for a trailing NULL
    char m_pWriteBuf[XML_WRITE_BUFFER_SIZE];

    uint8_t* m_pReadPtr;
    char* m_pWritePtr;  // write pointer within m_pBuf

    bool m_bUnicode;       // true = 16-bits, false = 8-bits
    bool m_bReverseBytes;  // true = reverse bytes, false = don't reverse

    bool m_bSkipNextAdvance;
    char m_Ch;  // Current character being parsed
};

}  // namespace ATG

#endif
