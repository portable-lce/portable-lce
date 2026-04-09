#pragma once
#include <string>
#include <unordered_map>

class File;

class Settings {
    //    public static Logger logger = Logger.getLogger("Minecraft");
    //    private Properties properties = new Properties();
private:
    std::unordered_map<std::string, std::string>
        properties;  // 4J - TODO was Properties type, will need to implement
                     // something we can serialise/deserialise too
                     // File *file;

public:
    Settings(File* file);
    void generateNewProperties();
    void saveProperties();
    std::string getString(const std::string& key,
                          const std::string& defaultValue);
    int getInt(const std::string& key, int defaultValue);
    bool getBoolean(const std::string& key, bool defaultValue);
    void setBooleanAndSave(const std::string& key, bool value);
};
