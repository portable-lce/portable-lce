#pragma once

#include <string>

// 4J Stu - The java formated numbers based on a local passed in, but I am just
// going for a constant format here
class NumberFormat {
public:
    static std::string format(int value) {
        // TODO 4J Stu - Change the length of the formatted number
        char output[256];
        snprintf(output, 256, "%d", value);
        std::string result = std::string(output);
        return result;
    }
};

class DecimalFormat {
private:
    const std::string formatString;

public:
    std::string format(double value) {
        // TODO 4J Stu - Change the length of the formatted number
        char output[256];
        snprintf(output, 256, formatString.c_str(), value);
        std::string result = std::string(output);
        return result;
    }

    // 4J Stu - The java code took a string format, we take a printf format
    // string
    DecimalFormat(std::string x) : formatString(x) {};
};