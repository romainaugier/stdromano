#if !defined(__STDROMANO_SPDLOGUTILS)
#define __STDROMANO_SPDLOGUTILS

#include "stdromano/stdromano.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"


STDROMANO_NAMESPACE_BEGIN

enum LogLevel : uint8_t
{
    Error = spdlog::level::err,
    Warning = spdlog::level::warn,
    Info = spdlog::level::info,
    Debug = spdlog::level::debug,
};

class STDROMANO_API Logger
{
public:
    static Logger& get_instance() noexcept
    {
        static Logger c;
        return c;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template <typename... Args>
    void error(Args&&... args) noexcept
    {
        this->_logger.error(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(Args&&... args) noexcept
    {
        this->_logger.warn(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(Args&&... args) noexcept
    {
        this->_logger.info(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(Args&&... args) noexcept
    {
        this->_logger.debug(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void trace(Args&&... args) noexcept
    {
        this->_logger.trace(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(Args&&... args) noexcept
    {
        this->_logger.critical(std::forward<Args>(args)...);
    }

    void set_level(uint8_t log_level) noexcept
    {
        this->_logger.set_level(static_cast<spdlog::level::level_enum>(log_level));
        this->_logger.sinks()[0]->set_level(static_cast<spdlog::level::level_enum>(log_level));
    }

private:
    Logger();
    ~Logger();

    spdlog::logger _logger;
};

#define log_error(...) Logger::get_instance().error(__VA_ARGS__)
#define log_warn(...) Logger::get_instance().warn(__VA_ARGS__)
#define log_info(...) Logger::get_instance().info(__VA_ARGS__)
#define log_debug(...) Logger::get_instance().debug(__VA_ARGS__)
#define log_trace(...) Logger::get_instance().trace(__VA_ARGS__)
#define log_critical(...) Logger::get_instance().critical(__VA_ARGS__)
#define set_log_level(level) Logger::get_instance().set_level(level)

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_SPDLOGUTILS) */