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

    const char* text = "import os\nprint(os.curdir)\nvar = 10\nvar += 10\ndef add(a, b):\n    return a + b\n";

    stdromano::Python::AST ast(logger);

    if(!ast.from_text(text, true))
        return 1;

    spdlog::info("Finished Python test");

    return 0;
}
