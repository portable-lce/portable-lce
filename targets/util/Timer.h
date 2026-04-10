#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <print>
#include <source_location>
#include <string_view>

namespace time_util {

using clock = std::chrono::steady_clock;
using time_point = clock::time_point;

template <typename T>
concept Duration = requires {
    typename T::rep;
    typename T::period;
};

namespace detail {

[[nodiscard]] constexpr auto base_name(std::string_view path) noexcept
    -> std::string_view {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string_view::npos ? path : path.substr(pos + 1);
}

}  // namespace detail

class Timer final {
public:
    Timer() noexcept : start_(clock::now()) {}

    void reset() noexcept { start_ = clock::now(); }

    [[nodiscard]] auto elapsed() const noexcept -> clock::duration {
        return clock::now() - start_;
    }

    template <Duration D = std::chrono::duration<double>>
    [[nodiscard]] auto elapsed_as() const noexcept -> D {
        return std::chrono::duration_cast<D>(elapsed());
    }

    [[nodiscard]] auto elapsed_seconds() const noexcept -> double {
        return elapsed_as<std::chrono::duration<double>>().count();
    }

    [[nodiscard]] auto elapsed_millis() const noexcept -> double {
        return elapsed_as<std::chrono::duration<double, std::milli>>().count();
    }

private:
    time_point start_;
};

class [[nodiscard]] ScopedTimer final {
public:
    explicit ScopedTimer(
        std::string_view name,
        std::source_location where = std::source_location::current())
        : name_(name),
          file_(detail::base_name(where.file_name())),
          line_(where.line()) {}

    template <Duration D>
    ScopedTimer(std::string_view name, D min_duration_to_log,
                std::source_location where = std::source_location::current())
        : name_(name),
          file_(detail::base_name(where.file_name())),
          line_(where.line()),
          min_duration_to_log_(std::chrono::duration_cast<clock::duration>(
              min_duration_to_log)) {}

    ~ScopedTimer() noexcept {
        const auto elapsed = timer_.elapsed();
        if (elapsed < min_duration_to_log_) return;

        const auto ms =
            std::chrono::duration<double, std::milli>(elapsed).count();

        try {
            const auto log =
                name_.empty() ? std::format("[TIMER] {:.3f} ms ({}:{})\n", ms,
                                            file_, line_)
                              : std::format("[TIMER] {} - {:.3f} ms ({}:{})\n",
                                            name_, ms, file_, line_);
            std::fputs(log.c_str(), stderr);
        } catch (...) {
            std::fprintf(stderr, "[TIMER] %.3f ms\n", ms);
        }
    }

    ScopedTimer(const ScopedTimer&) = delete;
    auto operator=(const ScopedTimer&) -> ScopedTimer& = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    auto operator=(ScopedTimer&&) -> ScopedTimer& = delete;

private:
    std::string_view name_;
    std::string_view file_;
    std::uint_least32_t line_;
    clock::duration min_duration_to_log_{clock::duration::zero()};
    Timer timer_;
};

}  // namespace time_util
