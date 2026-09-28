// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#if defined(STDROMANO_WIN)
#include <processenv.h>
#endif /* defined(STDROMANO_WIN) */

#include "stdromano/command_line_parser.hpp"
#include "stdromano/vector.hpp"

#include <cstring>

STDROMANO_NAMESPACE_BEGIN

#if defined(STDROMANO_WIN)
// Inverse of CommandLineToArgvW: https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
static void append_quoted_argument(StringD& command, const char* arg) noexcept
{
    if(*arg != '\0' && std::strpbrk(arg, " \t\n\v\"") == nullptr)
    {
        command.appendc(arg);
        return;
    }

    command.push_back('"');

    for(const char* c = arg;; c++)
    {
        std::size_t backslashes = 0;

        while(*c == '\\')
        {
            backslashes++;
            c++;
        }

        if(*c == '\0')
        {
            // Doubled so the closing quote is not escaped
            for(std::size_t i = 0; i < backslashes * 2; i++)
                command.push_back('\\');

            break;
        }

        if(*c == '"')
        {
            for(std::size_t i = 0; i < backslashes * 2 + 1; i++)
                command.push_back('\\');
        }
        else
        {
            for(std::size_t i = 0; i < backslashes; i++)
                command.push_back('\\');
        }

        command.push_back(*c);
    }

    command.push_back('"');
}
#elif defined(STDROMANO_UNIX)
static void append_quoted_argument(StringD& command, const char* arg) noexcept
{
    const char* safe_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@%+=:,./-_";

    if(*arg != '\0' && arg[std::strspn(arg, safe_chars)] == '\0')
    {
        command.appendc(arg);
        return;
    }

    command.push_back('\'');

    for(const char* c = arg; *c != '\0'; c++)
    {
        if(*c == '\'')
            command.appendc("'\\''");
        else
            command.push_back(*c);
    }

    command.push_back('\'');
}
#endif /* defined(STDROMANO_WIN) */

CommandLineParser::CommandLineParser()
{
}

CommandLineParser::~CommandLineParser()
{
}

Expected<void> CommandLineParser::add_argument(const StringD& arg_name,
                                               std::uint32_t arg_type,
                                               std::uint32_t arg_mode,
                                               const StringD& arg_short_name) noexcept
{
    if(this->_args.find(arg_name) != this->_args.end())
        return Error(StringD::make_fmt("Argument \"{}\" is already declared in the command line parser", arg_name));

    this->_args[arg_name] = CommandLineArg(arg_name, arg_type, arg_mode);

    if(!arg_short_name.empty())
        this->_aliases.emplace(arg_short_name, arg_name);

    return Ok();
}

Expected<void> CommandLineParser::parse(int argc, char** argv) noexcept
{
    for(std::int32_t i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if(arg[0] == '-' && arg[1] == '-' && arg[2] == '\0')
        {
            if(i + 1 < argc)
            {
                StringD after_args;

                for(std::int32_t j = i + 1; j < argc; j++)
                {
                    if(j > i + 1)
                        after_args.push_back(' ');

                    append_quoted_argument(after_args, argv[j]);
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
            const auto alias_it = this->_aliases.find(key);

            if(alias_it == this->_aliases.end())
                continue;

            key = alias_it->second;
            arg_it = this->_args.find(key);
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
                        value_end++;

                    if(*value_end != quote)
                        return Error(StringD::make_fmt("Cannot find closing quote for argument: {}",
                                                       fmt::string_view(key_start, key_len)));

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
                return Error(StringD::make_fmt("Argument \"{}\" requires a value but none provided",
                                               fmt::string_view(key_start, key_len)));
            }

            this->_args[std::move(key)].set_data(value, value_len);
        }
    }

    return Ok();
}

STDROMANO_NAMESPACE_END
