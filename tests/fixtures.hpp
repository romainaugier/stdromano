// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_TESTS_FIXTURES)
#define __STDROMANO_TESTS_FIXTURES

#include "stdromano/fuzz.hpp"
#include "stdromano/simd.hpp"
#include "stdromano/string.hpp"
#include "stdromano/test.hpp"

#include "spdlog/spdlog.h"

#include <cstdint>
#include <string>
#include <utility>

namespace fixtures {

class Tracked
{
    std::string _value;
    bool _moved_from = false;

    static inline std::int64_t _live = 0;

public:
    Tracked()
    {
        ++Tracked::_live;
    }

    Tracked(const char* value) : _value(value)
    {
        ++Tracked::_live;
    }

    Tracked(std::string value) : _value(std::move(value))
    {
        ++Tracked::_live;
    }

    Tracked(const stdromano::StringD& value) : _value(value.c_str(), value.size())
    {
        ++Tracked::_live;
    }

    Tracked(const Tracked& other) : _value(other._value)
    {
        ++Tracked::_live;
    }

    Tracked(Tracked&& other) noexcept : _value(std::move(other._value))
    {
        other._moved_from = true;
        ++Tracked::_live;
    }

    Tracked& operator=(const Tracked& other)
    {
        this->_value = other._value;
        this->_moved_from = false;
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept
    {
        if(this != &other)
        {
            this->_value = std::move(other._value);
            this->_moved_from = false;
            other._moved_from = true;
        }

        return *this;
    }

    ~Tracked()
    {
        --Tracked::_live;
    }

    bool operator==(const Tracked& other) const noexcept
    {
        return this->_value == other._value;
    }

    bool operator!=(const Tracked& other) const noexcept
    {
        return !(*this == other);
    }

    const std::string& value() const noexcept
    {
        return this->_value;
    }

    bool moved_from() const noexcept
    {
        return this->_moved_from;
    }

    static std::int64_t live() noexcept
    {
        return Tracked::_live;
    }
};

class QuietLogs
{
    spdlog::level::level_enum _previous;

public:
    QuietLogs() : _previous(spdlog::get_level())
    {
        spdlog::set_level(spdlog::level::off);
    }

    ~QuietLogs()
    {
        spdlog::set_level(this->_previous);
    }
};

inline stdromano::fuzz::Options options(const char* name, std::uint64_t iterations)
{
    stdromano::fuzz::Options opts;
    opts.name = name;
    opts.iterations = iterations;

#if defined(DEBUG_BUILD)
    opts.iterations = iterations / 4 + 1;
#endif // defined(DEBUG_BUILD)

    return opts;
}

inline bool is_debug_build() noexcept
{
#if defined(DEBUG_BUILD)
    return true;
#else
    return false;
#endif // defined(DEBUG_BUILD)
}

template <typename F>
void for_each_vectorization_mode(F&& func)
{
    const std::uint32_t previous = stdromano::simd_get_vectorization_mode();

    for(std::uint32_t mode = stdromano::VectorizationMode_Scalar;
        mode < stdromano::VectorizationMode_Max;
        ++mode)
    {
        if(!stdromano::simd_mode_is_available(mode))
            continue;

        stdromano::simd_force_vectorization_mode(mode);

        func(mode);
    }

    stdromano::simd_force_vectorization_mode(previous);
}

} // namespace fixtures

#define TESTS_REQUIRE_PROPERTY(report) STDROMANO_REQUIRE_MSG((report).passed(), (report).describe())

#endif // !defined(__STDROMANO_TESTS_FIXTURES)
