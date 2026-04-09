#pragma once

#include <cstdarg>
#include <cstdio>

namespace Log {

#if defined(_FINAL_BUILD)
// In shipping builds, info traces compile to nothing - matches the
// historical Game::DebugPrintf behaviour. The varargs path is
// completely elided so format strings and their arguments are not
// even constructed.
inline void info(const char* /*fmt*/, ...) {}
#else
#if defined(__GNUC__) || defined(__clang__)
[[gnu::format(printf, 1, 2)]]
#endif
inline void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
}
#endif

}  // namespace Log
