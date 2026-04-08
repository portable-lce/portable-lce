
#include "minecraft/world/level/WstringLookup.h"

#include <utility>

WstringLookup::WstringLookup() { numIDs = 0; }

std::string WstringLookup::lookup(unsigned int id) {
    // TODO
    // if (id > currentMaxID)
    //	throw error

    return int2str.at(id);
}

unsigned int WstringLookup::lookup(std::string str) {
    if (str2int.find(str) == str2int.end()) {
        std::pair<std::string, unsigned int> p =
            std::pair<std::string, unsigned int>(str, numIDs);

        str2int.insert(p);
        int2str.push_back(str);

        return numIDs++;
    } else {
        return str2int.at(str);
    }
}

void WstringLookup::getTable(std::string** lookup, unsigned int* len) {
    // Outputs
    std::string* out_lookup;
    unsigned int out_len;

    // Fill lookup.
    out_lookup = new std::string[int2str.size()];
    for (unsigned int i = 0; i < numIDs; i++) out_lookup[i] = int2str.at(i);

    out_len = numIDs;

    // Return.
    *lookup = out_lookup;
    *len = out_len;
    return;
}
