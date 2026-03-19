// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/python.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/filesystem.hpp"

#define STDROMANO_ENABLE_PROFILING
#include "stdromano/profiling.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"

int main()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");

    auto logger = std::make_shared<spdlog::logger>("python", console_sink);
    logger->set_level(spdlog::level::trace);

    spdlog::info("Starting Python test");

    stdromano::Python::AST ast(logger);

    const stdromano::StringD source_code_path("{}/python/test_all.py", TESTS_DATA_DIR);

    if(!stdromano::fs_path_exists(source_code_path))
    {
        spdlog::warn("Source code file does not exist, discarding test");
        return 0;
    }

    const stdromano::StringD source_code = stdromano::load_file_content(source_code_path);

    if(source_code.empty())
    {
        spdlog::error("Cannot load source code from file content");
        return 1;
    }

    if(!ast.from_text(source_code, true))
    {
        spdlog::error("Cannot parse source code");
        return 1;
    }

    spdlog::info("Finished Python test");

    return 0;
}
