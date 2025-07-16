// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/command_line_parser.hpp"
#include "stdromano/logger.hpp"

int main(int argc, char** argv)
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    stdromano::log_info("Starting command line parser test");

    stdromano::CommandLineParser cmd_line_parser;
    cmd_line_parser.add_argument("str_arg", stdromano::ArgType_String);
    cmd_line_parser.add_argument("int_arg", stdromano::ArgType_Int);
    cmd_line_parser.add_argument("bool_arg", stdromano::ArgType_Bool);
    cmd_line_parser.add_argument("bool_store_true", stdromano::ArgType_Bool, stdromano::ArgMode_StoreTrue);
    cmd_line_parser.add_argument("another-int-arg", stdromano::ArgType_Int);

    cmd_line_parser.parse(argc, argv);

    stdromano::log_debug("Argument str_arg value: {}", cmd_line_parser.get_argument_value<stdromano::StringD>("str_arg"));
    stdromano::log_debug("Argument int_arg value: {}", cmd_line_parser.get_argument_value<int>("int_arg"));
    stdromano::log_debug("Argument bool_arg value: {}", cmd_line_parser.get_argument_value<bool>("bool_arg"));
    stdromano::log_debug("Argument bool_store_true: {}", cmd_line_parser.get_argument_value<bool>("bool_store_true"));
    stdromano::log_debug("Argument another-int-arg value: {}", cmd_line_parser.get_argument_value<int>("another-int-arg"));
    stdromano::log_debug("Command to execute after arguments: \"{}\"", cmd_line_parser.get_command_after_args());

    stdromano::log_info("Finished command line parser test");

    return 0;
}