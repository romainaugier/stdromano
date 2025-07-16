// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_CMDLINEPARSER)
#define __STDROMANO_CMDLINEPARSER

#include "stdromano/string.hpp"
#include "stdromano/hashmap.hpp"

STDROMANO_NAMESPACE_BEGIN

enum ArgType : std::uint32_t
{
    ArgType_Bool = 0,
    ArgType_Int = 1,
    ArgType_String = 2,
};

enum ArgMode : std::uint32_t
{
    ArgMode_Store = 0,
    ArgMode_StoreTrue = 1,
    ArgMode_StoreFalse = 2,
};

class CommandLineArg
{
    StringD _name;
    StringD _data;

    std::uint32_t _type;
    std::uint32_t _mode;

public:
    CommandLineArg() = default;

    CommandLineArg(const StringD& arg_name,
                   std::uint32_t arg_type,
                   std::uint32_t arg_mode = ArgMode::ArgMode_Store) : _name(arg_name),
                                                                      _type(arg_type),
                                                                      _mode(arg_mode) {}

    STDROMANO_FORCE_INLINE void set_data(const char* arg_data, const size_t len) noexcept
    {
        this->_data = StringD(arg_data, len);
    }

    STDROMANO_FORCE_INLINE const StringD& get_name() const noexcept { return this->_name; }

    STDROMANO_FORCE_INLINE std::uint32_t get_type() const noexcept { return this->_type; }

    STDROMANO_FORCE_INLINE std::uint32_t get_mode() const noexcept { return this->_mode; }

    STDROMANO_FORCE_INLINE const StringD& get_data() const noexcept { return this->_data; }

    template<typename T>
    std::enable_if_t<!std::is_same_v<T, StringD> &&
                     !std::is_same_v<T, bool> &&
                     !std::is_integral_v<T> &&
                     !std::is_floating_point_v<T>,
                     T>
    get_data_as() const noexcept = delete; 

    template<typename T>
    std::enable_if_t<std::is_same_v<T, StringD>, T>
    get_data_as() const noexcept { return this->_data; }

    template<typename T>
    std::enable_if_t<std::is_same_v<T, bool>, T>
    get_data_as() const noexcept { return this->_data.to_bool(); }

    template<typename T> 
    std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<bool, T>, T>
    get_data_as() const noexcept { return static_cast<T>(this->_data.to_long_long()); }

    template<typename T>
    std::enable_if_t<std::is_floating_point_v<T>, T>
    get_data_as() const noexcept { return static_cast<T>(this->_data.to_double()); }
};

class STDROMANO_API CommandLineParser
{
    std::unordered_map<StringD, CommandLineArg> _args;
    std::unordered_map<StringD, StringD> _aliases;

    StringD _command_after_args;

public:
    CommandLineParser();

    ~CommandLineParser();

    void add_argument(const StringD& arg_name, 
                      std::uint32_t arg_type,
                      std::uint32_t arg_mode = ArgMode_Store,
                      const StringD& arg_alias = StringD()) noexcept;

    void parse(int argc, char** argv) noexcept;

    STDROMANO_FORCE_INLINE bool has_parsed_argument(const StringD& arg_name) const noexcept { return this->_args.find(arg_name)->second.get_data().size() > 0; }

    STDROMANO_FORCE_INLINE bool has_command_after_args() const noexcept { return !this->_command_after_args.empty(); }

    STDROMANO_FORCE_INLINE const StringD& get_command_after_args() const noexcept { return this->_command_after_args; }

    template<typename T>
    T get_argument_value(const StringD& arg_name, T default_value = {}) const noexcept
    {
        auto it = this->_args.find(arg_name);

        return it == this->_args.end() ? default_value : it->second.get_data_as<T>();
    }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_CMDLINEPARSER) */