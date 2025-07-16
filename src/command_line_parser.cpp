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

enum CmdLineTokenType : std::uint32_t
{
    CmdLineTokenType_Arg = 0,
    CmdLineTokenType_Value = 1,
    CmdLineTokenType_AfterArgs = 2
};

class CmdLineToken
{
    const char* _data;
    std::uint32_t _size;
    std::uint32_t _type;

public:
    CmdLineToken(std::uint32_t type, const char* data, std::uint32_t size) : _type(type),
                                                                             _data(data),
                                                                             _size(size) {}

    CmdLineToken(CmdLineToken& other) : _type(other._type),
                                        _data(other._data),
                                        _size(other._size) {}

    STDROMANO_FORCE_INLINE std::uint32_t get_type() const noexcept { return this->_type; }
    STDROMANO_FORCE_INLINE const char* get_data() const noexcept { return this->_data; }
    STDROMANO_FORCE_INLINE std::uint32_t get_size() const noexcept { return this->_size; }
};

bool command_line_lex(char* command_line, Vector<CmdLineToken>& tokens) noexcept
{
    char* s = command_line;

    while(*s != '\0')
    {
        if(*s == '-')
        {
            /* We reached the end of command line and parse what is after the final -- */
            if(*(s + 1) == '-' && *(s + 2) != '\0' && *(s + 2) == ' ')
            {
                s += 3;

                char* start = s;
                size_t size = 1;

                while(*s != '\0')
                {
                    s++;
                    size++;
                }

                if(*(s - 1) == ' ')
                {
                    size -= 2;
                }

                tokens.emplace_back(CmdLineTokenType_AfterArgs, start, size);

                return true;
            }

            while(*s == '-') s++;

            char* start = s;
            size_t size = 0;

            bool found_assign = false;

            while(1)
            {
                if(*s == '\0')
                {
                    break;
                }
                else if(*s == '=' || *s == ':')
                {
                    found_assign = true;
                    break;
                }
                else if(*s == ' ')
                {
                    break;
                }

                s++;
                size++;
            }

            tokens.emplace_back(CmdLineTokenType_Arg, start, size);

            s++;

            if(!found_assign)
            {
                continue;
            }

            char* value_start = s;
            size_t value_size = 0;

            if(*s == '"' || *s == '\'')
            {
                s++;

                /* We dont include the quotes in the argument value */
                value_start++;

                while(*s != '"' && *s != '\'')
                {
                    s++;
                    value_size++;

                    if(*s == '\0') 
                    {
                        const size_t error_pos = static_cast<size_t>(s - command_line);
                        log_error("Command line lexing error");
                        log_error("{}", command_line);
                        log_error("{:>{}}", "^", error_pos - 1);
                        log_error("Cannot find end of string for argument: {:.{}}", start, size);
                        return false;
                    }
                }
            }
            else
            {
                while(*s != ' ' && *s != '\0')
                {
                    s++;
                    value_size++;
                }
            }

            tokens.emplace_back(CmdLineTokenType_Value, value_start, value_size);
        }
        else
        {
            s++;
        }
    }

    return true;
}

void CommandLineParser::parse(int argc, char** argv) noexcept
{
    StringD command_line_args;
    
    for(size_t i = 1; i < static_cast<size_t>(argc); i++)
    {
        log_debug("argv[{}]: {}", i, argv[i]);

        if(i > 1)
        {
            command_line_args.push_back(' ');
        }

        command_line_args.appendc(argv[i]);
    }

    Vector<CmdLineToken> tokens;

    if(!command_line_lex(command_line_args.c_str(), tokens))
    {
        log_error("Error during command line arguments lexing, check the log for more informations");
        return;
    }

    for(size_t i = 0; i < tokens.size(); i++)
    {
        switch (tokens[i].get_type())
        {
            case CmdLineTokenType_Arg:
                {
                    String key;
                    key = StringD(tokens[i].get_data(),
                                  static_cast<std::size_t>(tokens[i].get_size()));

                    auto arg_it = this->_args.find(key);

                    if(arg_it == this->_args.end())
                    {
                        log_warn("Argument \"{}\" found in command line string but not declared in the command line parser", tokens[i].get_data());
                    }
                    else if(arg_it->second.get_mode() == ArgMode::ArgMode_StoreTrue)
                    {
                        this->_args[key].set_data("true", 4);
                    }
                    else if(arg_it->second.get_mode() == ArgMode::ArgMode_StoreFalse)
                    {
                        this->_args[key].set_data("false", 5);
                    }
                    else if((i + 1) < tokens.size() && tokens[i + 1].get_type() == CmdLineTokenType_Value)
                    {
                        this->_args[key].set_data(tokens[i + 1].get_data(),
                                                  static_cast<std::size_t>(tokens[i + 1].get_size()));

                        i++;
                    }
                }

                break;
            case CmdLineTokenType_AfterArgs:
                this->_command_after_args = StringD(tokens[i].get_data(),
                                                    static_cast<std::size_t>(tokens[i].get_size()));
                break;
            
            default:
                break;
        }
    }
}

STDROMANO_NAMESPACE_END