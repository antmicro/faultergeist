// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdarg>

namespace fin {

class Logger {
   public:
    enum class Level { Trace, Info, Warning, Error, Fatal };

    static Logger& get();
    bool enabled(Level level) const { return level == Level::Fatal || level >= threshold_; }
    Logger withTime() const { return Logger{threshold_, true}; }
    Logger withoutTime() const { return Logger{threshold_, false}; }
    Logger withIndent(int indent) const {
        Logger copy = *this;
        copy.indent_ = indent;
        return copy;
    }

    void trace(const char* file, int line, const char* format, ...) const;
    void info(const char* file, int line, const char* format, ...) const;
    void warning(const char* file, int line, const char* format, ...) const;
    void error(const char* file, int line, const char* format, ...) const;
    void fatal(const char* file, int line, const char* format, ...) const;

   private:
    Logger();
    Logger(Level threshold, bool timed) : threshold_(threshold), timed_(timed) {}
    void write(Level level, const char* file, int line, const char* format, va_list args) const;

    Level threshold_;
    bool timed_ = false;
    int indent_ = 0;
};

}  // namespace fin

#define FI_LOG(level, method, options, ...) \
    do { \
        auto& fi_logger = ::fin::Logger::get(); \
        if (fi_logger.enabled(::fin::Logger::Level::level)) \
            fi_logger.options.method(__FILE__, __LINE__, __VA_ARGS__); \
    } while (false)

#define FI_TRACE(...) FI_LOG(Trace, trace, withTime(), __VA_ARGS__)
#define FI_TRACE_UNTIMED(...) FI_LOG(Trace, trace, withoutTime(), __VA_ARGS__)
#define FI_TRACE_UNTIMED_INDENT(indent, ...) \
    FI_LOG(Trace, trace, withoutTime().withIndent(indent), __VA_ARGS__)
#define FI_INFO(...) FI_LOG(Info, info, withoutTime(), __VA_ARGS__)
#define FI_INFO_TIMED(...) FI_LOG(Info, info, withTime(), __VA_ARGS__)
#define FI_WARNING(...) FI_LOG(Warning, warning, withoutTime(), __VA_ARGS__)
#define FI_WARNING_INDENT(indent, ...) \
    FI_LOG(Warning, warning, withoutTime().withIndent(indent), __VA_ARGS__)
#define FI_ERROR(...) FI_LOG(Error, error, withoutTime(), __VA_ARGS__)
#define FI_ERROR_INDENT(indent, ...) \
    FI_LOG(Error, error, withoutTime().withIndent(indent), __VA_ARGS__)
#define FI_FATAL(...) FI_LOG(Fatal, fatal, withoutTime(), __VA_ARGS__)
