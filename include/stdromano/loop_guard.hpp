// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LOOP_GUARD)
#define __STDROMANO_LOOP_GUARD

#include "stdromano/stdromano.hpp"

#include <cstdlib>

STDROMANO_NAMESPACE_BEGIN

/// A guard for while-loops that prevents infinite iteration.
///
/// Usage:
///   LoopGuard guard("Parsing stuck in infinite loop");
///   while (guard--) {
///       // ...
///   }
///
/// If the loop exceeds the maximum number of iterations, the guard
/// triggers a debugger breakpoint (if attached) then aborts with
/// the provided diagnostic message.

class LoopGuard {
public:
    static constexpr std::size_t DEFAULT_MAX_ITER = 10000;

    constexpr LoopGuard(const char* message,
                        std::size_t max_iterations = DEFAULT_MAX_ITER) noexcept : _remaining(max_iterations),
                                                                                  _message(message) {}

    /// Post-decrement: returns true while iterations remain.
    /// Aborts with a diagnostic when the budget is exhausted.
    explicit operator bool() const noexcept { return this->_remaining > 0; }

    LoopGuard operator--(int) noexcept
    {
        LoopGuard snapshot = *this;

        if(this->_remaining == 0)
            this->on_limit_reached();

        --this->_remaining;

        return snapshot;
    }

    STDROMANO_NO_DISCARD std::size_t remaining() const noexcept { return this->_remaining; }

private:
    [[noreturn]] void on_limit_reached() const noexcept
    {
        // If a debugger is attached this will break right at the call-site
        // (look one frame up in the stack). If no debugger is attached the
        // signal is typically ignored or terminates, we call std::abort()
        // right after to guarantee termination either way.
#if defined(STDROMANO_MSVC)
        __debugbreak(); // MSVC / Windows
#elif defined(__has_builtin)
  #if __has_builtin(__builtin_debugtrap)
        __builtin_debugtrap(); // Clang
  #elif __has_builtin(__builtin_trap)
        __builtin_trap(); // GCC fallback
  #else
        __asm__ volatile("int $3"); // x86 fallback
  #endif
#elif defined(STDROMANO_GCC) && defined(STDROMANO_X86)
        __asm__ volatile("int $3"); // GCC on x86
#elif defined(STDROMANO_GCC) && defined(STDROMANO_AARCH64)
        __asm__ volatile("brk #0xF000"); // GCC on ARM64
#endif
        std::fprintf(stderr, "\n*** LoopGuard limit reached ***\n    %s\n\n", this->_message);
        std::abort();
    }

    std::size_t _remaining;
    const char* _message;
};

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_LOOP_GUARD)
