// SPDX-License-Identifier: Apache-2.0
#include "Logger.h"
#include "vpi_user.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace fin {
namespace {

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vpi_vprintf(const_cast<char*>(format), args);
    va_end(args);
}

}  // namespace

Logger& Logger::get() {
    static Logger logger;
    return logger;
}

Logger::Logger()
#ifdef NDEBUG
    : threshold_(Level::Info) {
#else
    : threshold_(Level::Trace) {
#endif
    const char* value = std::getenv("FI_LOG_LEVEL");
    if (!value) {
        return;
    }
    std::string level(value);
    std::transform(level.begin(), level.end(), level.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (level != "") {
        if (level == "trace") {
            threshold_ = Level::Trace;
        } else if (level == "info") {
            threshold_ = Level::Info;
        } else if (level == "warning") {
            threshold_ = Level::Warning;
        } else if (level == "error") {
            threshold_ = Level::Error;
        } else if (level == "fatal") {
            threshold_ = Level::Fatal;
        } else {
            print("[Warning] Invalid FI_LOG_LEVEL '%s'; using default\n", value);
        }
    }
}

void Logger::write(Level level, const char* file, int line, const char* format, va_list args)
    const {
    static const char* names[] = {"Trace", "Info", "Warning", "Error", "Fatal"};
    print("[%s]", names[static_cast<int>(level)]);
#ifndef NDEBUG
    print(" %s:%d", file, line);
#else
    (void)file;
    (void)line;
#endif
    if (timed_) {
        s_vpi_time time{};
        time.type = vpiSimTime;
        vpi_get_time(nullptr, &time);
        const auto ticks = (static_cast<std::uint64_t>(time.high) << 32) | time.low;
        print(" [@%llu]", static_cast<unsigned long long>(ticks));
    }
    print(" ");
    for (int i = 0; i < indent_; ++i) {
        print("\t");
    }
    vpi_vprintf(const_cast<char*>(format), args);
    print("\n");
    if (level == Level::Fatal) {
        vpi_control(vpiFinish);
    }
}

#define FI_DEFINE_METHOD(name, level) \
    void Logger::name(const char* file, int line, const char* format, ...) const { \
        va_list args; \
        va_start(args, format); \
        write(Level::level, file, line, format, args); \
        va_end(args); \
    }

FI_DEFINE_METHOD(trace, Trace)
FI_DEFINE_METHOD(info, Info)
FI_DEFINE_METHOD(warning, Warning)
FI_DEFINE_METHOD(error, Error)
FI_DEFINE_METHOD(fatal, Fatal)
#undef FI_DEFINE_METHOD

}  // namespace fin
