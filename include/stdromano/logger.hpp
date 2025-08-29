#pragma once

#if !defined(__STDROMANO_SPDLOGUTILS)
#define __STDROMANO_SPDLOGUTILS

#include "stdromano/stdromano.hpp"
#include "stdromano/string.hpp"

#include "spdlog/spdlog.h"

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

STDROMANO_API void log(std::uint32_t lvl, const StringD& msg) noexcept;

DETAIL_NAMESPACE_END

enum LogLevel : uint32_t
{
    Error = spdlog::level::err,
    Warning = spdlog::level::warn,
    Info = spdlog::level::info,
    Debug = spdlog::level::debug,
};

class Logger
{
public:
    STDROMANO_API static Logger& get_instance() noexcept;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template <typename... Args>
    void error(Args&&... args) noexcept
    {
        detail::log(spdlog::level::err, StringD::make_fmt(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(Args&&... args) noexcept
    {
        detail::log(spdlog::level::warn, StringD::make_fmt(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void info(Args&&... args) noexcept
    {
        detail::log(spdlog::level::info, StringD::make_fmt(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(Args&&... args) noexcept
    {
        detail::log(spdlog::level::debug, StringD::make_fmt(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void trace(Args&&... args) noexcept
    {
        detail::log(spdlog::level::trace, StringD::make_fmt(std::forward<Args>(args)...));
    }

    template <typename... Args>
    void critical(Args&&... args) noexcept
    {
        detail::log(spdlog::level::critical, StringD::make_fmt(std::forward<Args>(args)...));
    }

    STDROMANO_API void set_level(std::uint32_t log_level) noexcept;

    STDROMANO_API void flush() noexcept;

private:
    STDROMANO_API Logger();
    STDROMANO_API ~Logger();
};

#define log_error(...) Logger::get_instance().error(__VA_ARGS__)
#define log_warn(...) Logger::get_instance().warn(__VA_ARGS__)
#define log_info(...) Logger::get_instance().info(__VA_ARGS__)
#define log_debug(...) Logger::get_instance().debug(__VA_ARGS__)
#define log_trace(...) Logger::get_instance().trace(__VA_ARGS__)
#define log_critical(...) Logger::get_instance().critical(__VA_ARGS__)
#define set_log_level(level) Logger::get_instance().set_level(level)
#define log_flush() Logger::get_instance().flush()

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_SPDLOGUTILS) */
