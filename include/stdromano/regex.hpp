// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_REGEX)
#define __STDROMANO_REGEX

#include "stdromano/vector.hpp"
#include "stdromano/string.hpp"

#include <functional>

STDROMANO_NAMESPACE_BEGIN

#if defined(STDROMANO_GCC)
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif // defined(STDROMANO_GCC)

enum RegexFlags_ : std::uint32_t
{
    RegexFlags_DebugCompilation = 0x1,
};

static constexpr std::size_t REGEX_MAX_GROUPS = 16;

/* Represents a single captured group (or the full match when index is 0) */
struct STDROMANO_API RegexGroup
{
    std::size_t start = 0;
    std::size_t end = 0;

    RegexGroup() = default;
    RegexGroup(std::size_t s, std::size_t e) : start(s), end(e) {}

    STDROMANO_FORCE_INLINE std::size_t length() const noexcept { return this->end - this->start; }

    STDROMANO_FORCE_INLINE bool matched() const noexcept { return this->start != this->end; }
};

/* Result of a regex match operation */
class STDROMANO_API RegexMatch
{
private:
    StringD _source;
    RegexGroup _groups[REGEX_MAX_GROUPS];
    std::uint32_t _group_count;
    bool _matched;

public:
    RegexMatch() : _group_count(0), _matched(false) {}

    RegexMatch(const StringD& source,
               bool matched,
               const RegexGroup* groups,
               std::uint32_t group_count) : _source(source),
                                            _group_count(group_count),
                                            _matched(matched)
    {
        for(std::uint32_t i = 0; i < group_count && i < REGEX_MAX_GROUPS; ++i)
            this->_groups[i] = groups[i];
    }

    explicit operator bool() const noexcept { return this->_matched; }

    bool matched() const noexcept { return this->_matched; }

    RegexGroup group(std::uint32_t index) const noexcept
    {
        if(index < this->_group_count)
            return this->_groups[index];

        return RegexGroup();
    }

    /* Returns the matched substring for a given group */
    StringD group_str(std::uint32_t index) const noexcept
    {
        if(index < this->_group_count && this->_groups[index].matched())
            return StringD::make_ref(this->_source.data() + this->_groups[index].start,
                                     this->_groups[index].length());

        return StringD();
    }

    /* Shorthand: full match string (group 0) */
    StringD str() const noexcept { return this->group_str(0); }

    /* Start position of group 0 in the source string */
    std::size_t start() const noexcept { return this->_groups[0].start; }

    /* End position of group 0 in the source string */
    std::size_t end() const noexcept { return this->_groups[0].end; }

    std::uint32_t group_count() const noexcept { return this->_group_count; }
};

class STDROMANO_API Regex
{
public:
    using ByteCode = Vector<std::byte>;

private:
    ByteCode _bytecode;

    std::uint32_t _group_count;

    bool compile(const StringD& regex, std::uint32_t flags) noexcept;

    bool exec_at(const StringD& str, std::size_t start_pos, RegexGroup* groups) const noexcept;

public:
    Regex() : _group_count(0) {}

    Regex(const StringD& regex, std::uint32_t flags = 0) : _group_count(0)
    {
        this->compile(regex, flags);
    }

    RegexMatch match(const StringD& str) const noexcept;

    RegexMatch search(const StringD& str) const noexcept;

    void match_iter(const StringD& str,
                    const std::function<void(const RegexMatch&)>& callback) const noexcept;

    StringD replace_iter(const StringD& str,
                         const std::function<StringD(const RegexMatch&)>& callback) const noexcept;

    Vector<RegexMatch> match_all(const StringD& str) const noexcept;

    StringD replace_all(const StringD& str, const StringD& replace) const noexcept;

    bool valid() const noexcept { return !this->_bytecode.empty(); }

    std::uint32_t group_count() const noexcept { return this->_group_count; }
};

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_REGEX) */
