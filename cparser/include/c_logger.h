#ifndef _C_PARSER_C_LOGGER_H_
#define _C_PARSER_C_LOGGER_H_

#include <string>

#ifdef __GNUC__
#define printf_check(fmt, vara) __attribute__((format(printf, fmt, vara)))
#else
#define printf_check(fmt, vara)
#endif


namespace cparser {
class CLogger
{
public:
    enum LogLevel {
        LOG_LEVEL_DEBUG = 0,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_FATAL
    };

private:
    LogLevel m_logLevel;

protected:
    virtual void print_string(const char* str, size_t len);

public:
    CLogger(LogLevel logLevel = LOG_LEVEL_DEBUG);
    void set_log_level(LogLevel level);
    LogLevel get_log_level() const;

    virtual void debug(const char* fmt, ...) printf_check(2, 3);
    virtual void info (const char* fmt, ...) printf_check(2, 3);
    virtual void warn (const char* fmt, ...) printf_check(2, 3);
    virtual void error(const char* fmt, ...) printf_check(2, 3);
    virtual void fatal(const char* fmt, ...) printf_check(2, 3);
};
}

#endif // _C_PARSER_C_LOGGER_H_
