#include "UIString.h"

#include "app/common/Game.h"
#include "platform/XboxStubs.h"
#include "util/StringHelpers.h"

bool UIString::setCurrentLanguage() {
    int nextLanguage, nextLocale;
    nextLanguage = XGetLanguage();
    nextLocale = XGetLocale();

    if ((nextLanguage != s_currentLanguage) ||
        (nextLocale != s_currentLocale)) {
        s_currentLanguage = nextLanguage;
        s_currentLocale = nextLocale;
        return true;
    }

    return false;
}

int UIString::getCurrentLanguage() { return s_currentLanguage; }

UIString::UIStringCore::UIStringCore(StringBuilder wstrBuilder) {
    m_bIsConstant = false;

    m_lastSetLanguage = m_lastSetLocale = -1;
    m_lastUpdatedLanguage = m_lastUpdatedLocale = -1;

    m_fStringBuilder = wstrBuilder;

    m_wstrCache = "";
    update(true);
}

UIString::UIStringCore::UIStringCore(const std::string& str) {
    m_bIsConstant = true;

    m_lastSetLanguage = m_lastSetLocale = -1;
    m_lastUpdatedLanguage = m_lastUpdatedLocale = -1;

    m_wstrCache = str;
}

std::string& UIString::UIStringCore::getString() {
    if (hasNewString()) update(true);
    return m_wstrCache;
}

bool UIString::UIStringCore::hasNewString() {
    if (m_bIsConstant) return false;
    return (m_lastSetLanguage != s_currentLanguage) ||
           (m_lastSetLocale != s_currentLocale);
}

bool UIString::UIStringCore::update(bool force) {
    if (!m_bIsConstant && (force || hasNewString())) {
        m_wstrCache = m_fStringBuilder();
        m_lastSetLanguage = s_currentLanguage;
        m_lastSetLocale = s_currentLocale;
        return true;
    }
    return false;
}

bool UIString::UIStringCore::needsUpdating() {
    if (m_bIsConstant) return false;
    return (m_lastSetLanguage != s_currentLanguage) ||
           (m_lastUpdatedLanguage != m_lastSetLanguage) ||
           (m_lastSetLocale != s_currentLocale) ||
           (m_lastUpdatedLocale != m_lastSetLocale);
}

void UIString::UIStringCore::setUpdated() {
    m_lastUpdatedLanguage = m_lastSetLanguage;
    m_lastUpdatedLocale = m_lastSetLocale;
}

int UIString::s_currentLanguage = -1;
int UIString::s_currentLocale = -1;

UIString::UIString() { m_core = std::shared_ptr<UIStringCore>(); }

UIString::UIString(int ids) {
    StringBuilder builder = [ids]() { return app.GetString(ids); };
    UIStringCore* core = new UIStringCore(builder);
    m_core = std::shared_ptr<UIStringCore>(core);
}

UIString::UIString(StringBuilder wstrBuilder) {
    UIStringCore* core = new UIStringCore(wstrBuilder);
    m_core = std::shared_ptr<UIStringCore>(core);
}

// UIString::UIString(const std::wstring& constant) {
//     std::wstring wstr = convStringToWstring(constant);
//     UIStringCore* core = new UIStringCore(wstr);
//     m_core = std::shared_ptr<UIStringCore>(core);
// }

UIString::UIString(const std::string& constant) {
    UIStringCore* core = new UIStringCore(constant);
    m_core = std::shared_ptr<UIStringCore>(core);
}

UIString::UIString(const char* constant) {
    std::string str = std::string(constant);
    UIStringCore* core = new UIStringCore(str);
    m_core = std::shared_ptr<UIStringCore>(core);
}

UIString::~UIString() { m_core = nullptr; }

bool UIString::empty() { return m_core.get() == nullptr; }

bool UIString::compare(const UIString& uiString) {
    return m_core.get() != uiString.m_core.get();
}

bool UIString::needsUpdating() {
    if (m_core != nullptr)
        return m_core->needsUpdating();
    else
        return false;
}

void UIString::setUpdated() {
    if (m_core != nullptr) m_core->setUpdated();
}

std::string& UIString::getString() {
    static std::string blank("");
    if (m_core != nullptr)
        return m_core->getString();
    else
        return blank;
}

const char* UIString::c_str() { return getString().c_str(); }

unsigned int UIString::length() { return getString().length(); }