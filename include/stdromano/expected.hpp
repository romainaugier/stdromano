// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_EXPECTED)
#define __STDROMANO_EXPECTED

#include "stdromano/string.hpp"

// TODO: replace with better alternatives
#include <variant>
#include <optional>

STDROMANO_NAMESPACE_BEGIN

// Wrapper holding an error message as a string
struct Error
{
    StringD message;

    Error() = default;

    Error(StringD msg) : message(std::move(msg)) {}
};

// Rust's Result-like struct to handle error easily
template <typename T>
class Expected
{
public:
    Expected(T value) : _data(std::move(value)) {}

    Expected(Error err) : _data(std::move(err)) {}

    explicit operator bool() const
    {
        return std::holds_alternative<T>(this->_data);
    }

    bool has_value() const
    {
        return std::holds_alternative<T>(this->_data);
    }

    bool has_error() const
    {
        return std::holds_alternative<Error>(this->_data);
    }

    inline T value()
    {
        return std::move(std::get<T>(this->_data));
    }

    inline const T& value() const
    {
        return std::get<T>(this->_data);
    }

    inline T value_or(std::function<void(const Error&)> lbd)
    {
        if(this->has_error())
            lbd(this->error());

        return std::move(std::get<T>(this->_data));
    }

    inline T unwrap()
    {
        if(this->has_error())
            throw std::runtime_error("Trying to unwrap an errored Expected<T>");

        return std::move(std::get<T>(this->_data));
    }

    const Error& error() const
    {
        return std::get<Error>(this->_data);
    }

private:
    std::variant<T, Error> _data;
};

template <>
class Expected<void>
{
public:
    // Default construct as success
    Expected() : _error(std::nullopt) {}

    Expected(Error err) : _error(std::move(err)) {}

    explicit operator bool() const
    {
        return !this->_error.has_value();
    }

    bool has_value() const
    {
        return !this->_error.has_value();
    }

    bool has_error() const
    {
        return this->_error.has_value();
    }

    const Error& error() const
    {
        return *this->_error;
    }

    // Execute a callback if an error is present
    const Expected<void>& on_error(std::function<void(const Error&)> lbd) const
    {
        if(this->has_error())
            lbd(*this->_error);

        return *this;
    }

private:
    std::optional<Error> _error;
};

// Convenience Ok()
inline Expected<void> Ok()
{
    return Expected<void>{};
}

// Convenience macro
#define EXPECT(var, expr)                    \
    auto _res_##var = (expr)                 \
    if(!_res_##var.has_value())              \
        return _res_##var.error();           \
    auto var = std::move(_res_##var.value())

STDROMANO_NAMESPACE_END

template<typename T>
struct fmt::formatter<stdromano::Expected<T>>
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const stdromano::Expected<T>& e, format_context& ctx) const
    {
        return !e.has_error() ? format_to(ctx.out(), "Expected<T>: {}", e.value())
                              : format_to(ctx.out(), "Error: {}", e.error());
    }
};

#endif // !defined(__STDROMANO_EXPECTED)
