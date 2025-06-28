#include "c_logger.h"
#include <assert.h>
#include <map>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>
using namespace cparser;
using namespace std;
using LogLevel = CLogger::LogLevel;

#define LOG_BUF_SIZE 4096


static map<string, string> ansi_colors = {
    {"black", "\033[30m"},
    {"red", "\033[31m"},
    {"green", "\033[32m"},
    {"yellow", "\033[33m"},
    {"blue", "\033[34m"},
    {"magenta", "\033[35m"},
    {"cyan", "\033[36m"},
    {"white", "\033[37m"},
    {"reset", "\033[0m"},
};

#define clogger_func(fn, level, color)                                                             \
    void CLogger::fn(const char* fmt, ...)                                                         \
    {                                                                                              \
        if (this->m_logLevel > LogLevel::LOG_LEVEL_##level)                                        \
            return;                                                                                \
        const auto color_str = ansi_colors[#color];                                                \
        const auto reset_str = ansi_colors["reset"];                                               \
        char __buf[LOG_BUF_SIZE];                                                                  \
        char* buf = __buf;                                                                         \
        size_t buflen = sizeof(__buf);                                                             \
        const char* _buf = buf;                                                                    \
                                                                                                   \
        assert(color_str.size() < buflen);                                                         \
        memcpy(buf, color_str.c_str(), color_str.size());                                          \
        buflen -= color_str.size();                                                                \
        buf += color_str.size();                                                                   \
        assert(buflen > reset_str.size() + 2);                                                     \
        va_list args;                                                                              \
        va_start(args, fmt);                                                                       \
        auto len = vsnprintf(buf, buflen - reset_str.size() - 2, fmt, args);                       \
        buflen -= len;                                                                             \
        buf += len;                                                                                \
        va_end(args);                                                                              \
        memcpy(buf, reset_str.c_str(), reset_str.size());                                          \
        buflen -= reset_str.size();                                                                \
        buf += reset_str.size();                                                                   \
        buf++[0] = '\n';                                                                           \
        buf++[0] = '\0';                                                                           \
        buflen -= 2;                                                                               \
        this->print_string(_buf, sizeof(__buf) - buflen);                                          \
    }
clogger_func(debug, DEBUG, green);
clogger_func(info, INFO, blue);
clogger_func(warn, WARN, yellow);
clogger_func(error, ERROR, red);
clogger_func(fatal, FATAL, red);

CLogger::CLogger(LogLevel logLevel) : m_logLevel(logLevel)
{}

void CLogger::print_string(const char* str, size_t len)
{
    printf("%s", str);
}

void CLogger::set_log_level(LogLevel level)
{
    this->m_logLevel = level;
}
LogLevel CLogger::get_log_level() const
{
    return this->m_logLevel;
}
