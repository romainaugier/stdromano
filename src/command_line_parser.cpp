// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/command_line_parser.hpp"
#include "stdromano/logger.hpp"
#include "stdromano/vector.hpp"

#if defined(STDROMANO_WIN)
#include <processenv.h>
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_BEGIN

CommandLineParser::CommandLineParser()
{
}

CommandLineParser::~CommandLineParser()
{
}

void CommandLineParser::add_argument(const StringD& arg_name,
                                     std::uint32_t arg_type,
                                     std::uint32_t arg_mode,
                                     const StringD& arg_short_name) noexcept
{
    if(this->_args.find(arg_name) != this->_args.end())
    {
        log_warn("Argument \"{}\" is already declared in the command line parser", arg_name);
        return;
    }

    this->_args[arg_name] = CommandLineArg(arg_name, arg_type, arg_mode);

    if(arg_short_name != nullptr)
    {
        this->_aliases.emplace(arg_short_name, arg_name);
    }
}

void CommandLineParser::parse(int argc, char** argv) noexcept
{
    for(int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        log_debug("arg {}: {}", i, arg);

        if(arg[0] == '-' && arg[1] == '-' && arg[2] == '\0')
        {
            if(i + 1 < argc)
            {
                StringD after_args;

                for(int j = i + 1; j < argc; j++)
                {
                    if(j > i + 1)
                        after_args.push_back(' ');

                    after_args.appendc(argv[j]);
                }

                this->_command_after_args = std::move(after_args);
            }
            break;
        }

        if(arg[0] != '-')
            continue;

        const char* key_start = arg;

        while(*key_start == '-')
            key_start++;

        const char* key_end = key_start;
        const char* value_start = nullptr;

        bool has_inline_value = false;

        while(*key_end != '\0')
        {
            if(*key_end == ':' || *key_end == '=')
            {
                value_start = key_end + 1;
                has_inline_value = true;
                break;
            }

            key_end++;
        }

        size_t key_len = key_end - key_start;
        String key = StringD::make_from_c_str(key_start, key_len);

        auto arg_it = this->_args.find(key);

        if(arg_it == this->_args.end())
        {
            log_warn("Argument \"{}\" found in command line but not declared in the command line parser",
                     StringD(key_start, key_len).c_str());
            continue;
        }

        if(arg_it->second.get_mode() == ArgMode::ArgMode_StoreTrue)
        {
            this->_args[std::move(key)].set_data("true", 4);
        }
        else if(arg_it->second.get_mode() == ArgMode::ArgMode_StoreFalse)
        {
            this->_args[std::move(key)].set_data("false", 5);
        }
        else
        {
            const char* value = nullptr;
            size_t value_len = 0;

            if(has_inline_value)
            {
                value = value_start;

                if(*value == '"' || *value == '\'')
                {
                    char quote = *value;
                    value++;

                    const char* value_end = value;
                    while(*value_end != '\0' && *value_end != quote)
                    {
                        value_end++;
                    }

                    if(*value_end != quote)
                    {
                        log_error("Cannot find closing quote for argument: {}",
                                  StringD(key_start, key_len).c_str());
                        continue;
                    }

                    value_len = value_end - value;
                }
                else
                {
                    value_len = strlen(value);
                }
            }
            else if(i + 1 < argc)
            {
                i++;
                value = argv[i];
                value_len = strlen(value);
            }
            else
            {
                log_error("Argument \"{}\" requires a value but none provided",
                          StringD(key_start, key_len).c_str());
                continue;
            }

            this->_args[std::move(key)].set_data(value, value_len);
        }
    }
}

STDROMANO_NAMESPACE_END
