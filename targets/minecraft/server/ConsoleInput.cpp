#include "ConsoleInput.h"

class ConsoleInputSource;

ConsoleInput::ConsoleInput(const std::string& msg, ConsoleInputSource* source) {
    this->msg = msg;
    this->source = source;
}