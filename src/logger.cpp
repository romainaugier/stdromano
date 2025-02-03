#include "stdromano/logger.h"
#include "stdromano/memory.h"

STDROMANO_NAMESPACE_BEGIN

Logger::Logger() : _logger("null")
{
#if defined(STDROMANO_WIN)
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif /* defined(STDROMANO_WIN) */

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("%l : %v");

    char* log_level = std::getenv("STDROMANO_LOG_LEVEL");
    
    if(log_level == nullptr)
    {
        console_sink->set_level(spdlog::level::info);
    }
    else if(std::strcmp("error", log_level) == 0)
    {
        console_sink->set_level(spdlog::level::err);
    }
    else if(std::strcmp("warning", log_level) == 0)
    {
        console_sink->set_level(spdlog::level::warn);
    }
    else if(std::strcmp("info", log_level) == 0)
    {
        console_sink->set_level(spdlog::level::info);
    }
    else if(std::strcmp("debug", log_level) == 0)
    {
        console_sink->set_level(spdlog::level::debug);
    }

    char* log_file = std::getenv("STDROMANO_LOG_FILE");

    if(log_file == nullptr)
    {
        const char* temp = std::getenv("TEMP");

        size_t buf_size = std::snprintf(NULL, 0, "%s/stdromano_log.txt", temp) + 1;
        log_file = static_cast<char*>(mem_alloca(buf_size * sizeof(char)));
        std::snprintf(log_file, buf_size, "%s/stdromano_log.txt", temp);
    }

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, false);
    file_sink->set_pattern("[%l] %H:%M:%S:%e : %v");
    file_sink->set_level(spdlog::level::debug);

    this->_logger = spdlog::logger("main_logger", {console_sink, file_sink});
    this->_logger.set_level(spdlog::level::debug);

    spdlog::flush_on(spdlog::level::info);
}

Logger::~Logger()
{
    this->_logger.flush();
}

STDROMANO_NAMESPACE_END