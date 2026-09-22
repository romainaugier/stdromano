// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/test.hpp"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>

#if defined(STDROMANO_WIN)
#include <process.h>
#define STDROMANO_GETPID _getpid
#else
#include <unistd.h>
#define STDROMANO_GETPID getpid
#endif // defined(STDROMANO_WIN)

STDROMANO_NAMESPACE_BEGIN

namespace test {

// One counter per open scope, innermost last. A mutex rather than thread_local: failures
// reported from worker threads belong to the case the main thread is running
static std::mutex g_scopes_mutex;
static std::vector<std::size_t> g_scopes;
static std::atomic<std::size_t> g_failures_outside_scopes{0};

void report_failure(const char* file, int line, const char* message) noexcept
{
    spdlog::error("{}:{}: {}", file, line, message);

    std::lock_guard<std::mutex> lock(g_scopes_mutex);

    if(g_scopes.empty())
        g_failures_outside_scopes.fetch_add(1);
    else
        g_scopes.back()++;
}

void begin_scope() noexcept
{
    std::lock_guard<std::mutex> lock(g_scopes_mutex);
    g_scopes.push_back(0);
}

std::size_t end_scope() noexcept
{
    std::lock_guard<std::mutex> lock(g_scopes_mutex);

    if(g_scopes.empty())
        return 0;

    const std::size_t failures = g_scopes.back();
    g_scopes.pop_back();

    return failures;
}

bool in_scope() noexcept
{
    std::lock_guard<std::mutex> lock(g_scopes_mutex);
    return !g_scopes.empty();
}

static std::mutex g_temp_mutex;
static std::filesystem::path g_temp_dir;

static void remove_temp_dir()
{
    std::error_code ec;
    std::filesystem::remove_all(g_temp_dir, ec);
}

StringD temp_path(const char* name)
{
    std::lock_guard<std::mutex> lock(g_temp_mutex);

    if(g_temp_dir.empty())
    {
        g_temp_dir = std::filesystem::temp_directory_path() /
                     ("stdromano_test_" + std::to_string(STDROMANO_GETPID()));

        std::error_code ec;
        std::filesystem::remove_all(g_temp_dir, ec);
        std::filesystem::create_directories(g_temp_dir, ec);

        std::atexit(remove_temp_dir);
    }

    const std::string path = (g_temp_dir / name).string();

    return StringD::make_from_c_str(path.c_str(), path.size());
}

DETAIL_NAMESPACE_BEGIN

static bool is_option(const char* arg) noexcept { return arg != nullptr && arg[0] == '-'; }

bool matches_filter(const char* name, int argc, char** argv) noexcept
{
    bool has_filter = false;

    for(int i = 1; i < argc; ++i)
    {
        if(is_option(argv[i]))
            continue;

        has_filter = true;

        if(std::strstr(name, argv[i]) != nullptr)
            return true;
    }

    const char* env_filter = std::getenv("STDROMANO_TEST_FILTER");

    if(env_filter != nullptr && env_filter[0] != '\0')
    {
        has_filter = true;

        if(std::strstr(name, env_filter) != nullptr)
            return true;
    }

    return !has_filter;
}

bool wants_list(int argc, char** argv) noexcept
{
    for(int i = 1; i < argc; ++i)
        if(std::strcmp(argv[i], "--list") == 0)
            return true;

    return false;
}

void configure_logging(int argc, char** argv) noexcept
{
    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0)
        {
            spdlog::set_level(spdlog::level::debug);
            return;
        }
    }
}

DETAIL_NAMESPACE_END

} // namespace test

STDROMANO_NAMESPACE_END
