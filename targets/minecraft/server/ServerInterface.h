#pragma once

class ServerInterface {
    virtual int getConfigInt(const std::string& name, int defaultValue) = 0;
    virtual std::string getConfigString(const std::string& name,
                                        const std::string& defaultValue) = 0;
    virtual bool getConfigBoolean(const std::string& name,
                                  bool defaultValue) = 0;
    virtual void setProperty(std::string& propertyName, void* value) = 0;
    virtual void configSave() = 0;
    virtual std::string getConfigPath() = 0;
    virtual std::string getServerIp() = 0;
    virtual int getServerPort() = 0;
    virtual std::string getServerName() = 0;
    virtual std::string getServerVersion() = 0;
    virtual int getPlayerCount() = 0;
    virtual int getMaxPlayers() = 0;
    virtual std::string[] getPlayerNames() = 0;
    virtual std::string getWorldName() = 0;
    virtual std::string getPluginNames() = 0;
    virtual void disablePlugin() = 0;
    virtual std::string runCommand(const std::string& command) = 0;
    virtual bool isDebugging() = 0;
    // Logging
    virtual void info(const std::string& string) = 0;
    virtual void warn(const std::string& string) = 0;
    virtual void error(const std::string& string) = 0;
    virtual void debug(const std::string& string) = 0;
};