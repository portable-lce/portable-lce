#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class WstringLookup {
private:
    unsigned int numIDs;
    std::unordered_map<std::string, unsigned int> str2int;
    std::vector<std::string> int2str;

public:
    WstringLookup();

    std::string lookup(unsigned int id);

    unsigned int lookup(std::string);

    void getTable(std::string** lookup, unsigned int* len);
};
