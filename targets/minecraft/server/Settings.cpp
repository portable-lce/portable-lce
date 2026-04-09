#include "Settings.h"

#include "util/StringHelpers.h"

// 4J - TODO - serialise/deserialise from file
Settings::Settings(File* file) {}

void Settings::generateNewProperties() {}

void Settings::saveProperties() {}

std::string Settings::getString(const std::string& key,
                                const std::string& defaultValue) {
    if (properties.find(key) == properties.end()) {
        properties[key] = defaultValue;
        saveProperties();
    }
    return properties[key];
}

int Settings::getInt(const std::string& key, int defaultValue) {
    if (properties.find(key) == properties.end()) {
        properties[key] = toWString<int>(defaultValue);
        saveProperties();
    }
    return fromWString<int>(properties[key]);
}

bool Settings::getBoolean(const std::string& key, bool defaultValue) {
    if (properties.find(key) == properties.end()) {
        properties[key] = toWString<bool>(defaultValue);
        saveProperties();
    }
    bool retval = fromWString<bool>(properties[key]);
    return retval;
}

void Settings::setBooleanAndSave(const std::string& key, bool value) {
    properties[key] = toWString<bool>(value);
    saveProperties();
}