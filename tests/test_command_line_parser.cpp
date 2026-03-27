// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/command_line_parser.hpp"

#include "spdlog/spdlog.h"

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    spdlog::info("Starting command line parser test");

    stdromano::CommandLineParser cmd_line_parser;
    cmd_line_parser.add_argument("str_arg", stdromano::ArgType_String);
    cmd_line_parser.add_argument("int_arg", stdromano::ArgType_Int);
    cmd_line_parser.add_argument("bool_arg", stdromano::ArgType_Bool);
    cmd_line_parser.add_argument("bool_store_true", stdromano::ArgType_Bool, stdromano::ArgMode_StoreTrue);
    cmd_line_parser.add_argument("another-int-arg", stdromano::ArgType_Int);

    cmd_line_parser.parse(argc, argv);

    spdlog::debug("Argument str_arg value: {}", cmd_line_parser.get_argument_value<stdromano::StringD>("str_arg"));
    spdlog::debug("Argument int_arg value: {}", cmd_line_parser.get_argument_value<int>("int_arg"));
    spdlog::debug("Argument bool_arg value: {}", cmd_line_parser.get_argument_value<bool>("bool_arg"));
    spdlog::debug("Argument bool_store_true: {}", cmd_line_parser.get_argument_value<bool>("bool_store_true"));
    spdlog::debug("Argument another-int-arg value: {}", cmd_line_parser.get_argument_value<int>("another-int-arg"));
    spdlog::debug("Command to execute after arguments: \"{}\"", cmd_line_parser.get_command_after_args());

    spdlog::info("Finished command line parser test");

    return 0;
}
