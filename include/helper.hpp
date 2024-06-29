#ifndef HELPER_H
#define HELPER_H
// #include <backward.hpp>
#include <float.h>
#include <math.h>
#include <fmt/core.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <execinfo.h>
#include <filesystem>
#include <sched.h>
#include <span>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <x86intrin.h>
#include <cpptrace/cpptrace.hpp>

template <typename... T>
inline void throw_with_stacktrace(fmt::format_string<T...> fmt, T &&...args) {

    // using namespace backward;
    // StackTrace st;
    // st.load_here(32);
    // Printer p;
    // p.print(st, stderr);

    auto message = fmt::format(fmt, std::forward<T>(args)...);

    auto error_message = fmt::format(
        "{}\n{}", message, cpptrace::generate_trace().to_string(true));

    throw std::runtime_error(std::string(error_message));
}

template <typename T> void assert_equal(T actual, T expected) {
    if (expected != actual) {
        throw std::runtime_error(
            fmt::format("Expected: {}, Actual: {}", expected, actual));
    }
} // namespace helper

template <typename T> std::span<T, 1> span1(T *element) {
    return std::span<T, 1>{element, 1};
}

inline std::filesystem::path get_parent_path() {
    char dest[PATH_MAX];
    memset(dest, 0, sizeof(dest)); // readlink does not null terminate!
    if (readlink("/proc/self/exe", dest, PATH_MAX) == -1) {
        throw std::runtime_error("readlink failed");
    }

    auto path = std::filesystem::path(dest);
    auto parent_path = path.parent_path();

    return parent_path;
}

inline double_t timestamp() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

template <size_t Extent>
inline void initialize_timespan(std::span<uint64_t, Extent> span) {
    span[0] = 1;
    span[1] = timestamp();
}

template <size_t Extent>
inline void inject_timespan(std::span<uint64_t, Extent> span) {
    span[0] = span[0] + 1;
    span[span[0]] = timestamp();
}

inline __attribute__((always_inline)) std::string getenv_throw(std::string const &key) {
    char *val = getenv(key.c_str());
    if (val == NULL) {
        throw_with_stacktrace("Environment variable {} not set", key);
    }
    return std::string(val);
}

#endif