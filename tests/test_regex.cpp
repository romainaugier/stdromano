// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/regex.hpp"
#include "stdromano/logger.hpp"

int main(int argc, char** argv)
{
    stdromano::set_log_level(stdromano::LogLevel::Debug);

    stdromano::log_info("Starting Regex test");

    stdromano::Regex regex1("[0-9]*", stdromano::RegexFlags_DebugCompilation);

    assert(regex1.match("123456789"));
    assert(regex1.match("12345abcde"));
    assert(regex1.match("abcde12345"));

    stdromano::Regex regex2("[0-9]+", stdromano::RegexFlags_DebugCompilation);

    assert(regex2.match("123456789"));
    assert(regex2.match("12345abcde"));
    assert(regex2.match("1abcde"));
    assert(regex2.match("12abcde"));
    assert(!regex2.match("abcde12345"));

    stdromano::Regex regex3("a*b|cd", stdromano::RegexFlags_DebugCompilation);

    assert(regex3.match("aaaaaacd"));
    assert(regex3.match("abd"));
    assert(regex3.match("bd"));
    assert(regex3.match("cd"));
    assert(!regex3.match("aaaacacd"));

    stdromano::Regex regex4("a?([b-e])+", stdromano::RegexFlags_DebugCompilation);

    assert(regex4.match("abcdebcde"));
    assert(regex4.match("bcdebcde"));
    assert(!regex4.match("rbcdebcde"));

    stdromano::log_info("Finished Regex test");

    return 0;
}