// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_ATOMIC)
#define __STDROMANO_ATOMIC

#include "stdromano/stdromano.hpp"

#if defined(STDROMANO_WIN)
#include <Windows.h>
#include <intrin.h>
#endif /* defined(STDROMANO_WIN) */

#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

enum class MemoryOrder 
{
    Relaxed,
    Consume,
    Acquire,
    Release,
    AcqRel,
    SeqCst
};

#if defined(STDROMANO_LINUX)
constexpr int to_gcc_memory_order(MemoryOrder order) 
{
    switch (order) {
        case MemoryOrder::Relaxed: 
            return __ATOMIC_RELAXED;
        case MemoryOrder::Consume:
        case MemoryOrder::Acquire: 
            return __ATOMIC_ACQUIRE;
        case MemoryOrder::Release:
            return __ATOMIC_RELEASE;
        case MemoryOrder::AcqRel:
            return __ATOMIC_ACQ_REL;
        case MemoryOrder::SeqCst:
            return __ATOMIC_SEQ_CST;
        default: 
            return __ATOMIC_SEQ_CST;
    }
}
#endif /* defined(STDROMANO_LINUX) */

template<typename T>
using is_valid_for_atomic = std::conjunction<std::is_integral<T>,
                                             std::disjunction<std::bool_constant<sizeof(T) == 4>,
                                                              std::bool_constant<sizeof(T) == 8>>>;                                         

template<typename T>
class Atomic 
{
    static_assert(is_valid_for_atomic<T>::value, "T must be integral and of size 4|8");

private:
    volatile T _value;

public:
    Atomic() noexcept = default;
    constexpr Atomic(T value) noexcept : _value(value) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T load(STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4)
        {
            return _InterlockedOr(reinterpret_cast<volatile long*>(&this->_value), 0);
        }
        else if constexpr (sizeof(T) == 8)
        {
            return _InterlockedOr64(reinterpret_cast<volatile __int64*>(&this->_value), 0);
        }
#elif defined(STDROMANO_LINUX)
        return __atomic_load_n(&this->_value, to_gcc_memory_order(order));
#endif /* defined(STDROMANO_WIN) */
    }

    void store(T value, STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4)
        {
            _InterlockedExchange(reinterpret_cast<volatile long*>(&this->_value), value);
        }
        else if constexpr (sizeof(T) == 8)
        {
            _InterlockedExchange64(reinterpret_cast<volatile __int64*>(&this->_value), value);
        }
#elif defined(STDROMANO_LINUX)
        __atomic_store_n(&this->_value, value, to_gcc_memory_order(order));
#endif /* defined(STDROMANO_WIN) */
    }

    T exchange(T value, STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4)
        {
            return _InterlockedExchange(reinterpret_cast<volatile long*>(&this->_value), value);
        }
        else if constexpr (sizeof(T) == 8)
        {
            return _InterlockedExchange64(reinterpret_cast<volatile __int64*>(&this->_value), value);
        }
#elif defined(STDROMANO_LINUX)
        return __atomic_exchange_n(&this->_value, value, to_gcc_memory_order(order));
#endif /* defined(STDROMANO_WIN) */
    }

    T fetch_add(T arg,
                STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4)
        {
            return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&this->_value), arg);
        }
        else if constexpr (sizeof(T) == 8)
        {
            return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&this->_value), arg);
        }
#elif defined(STDROMANO_LINUX)
        return __atomic_fetch_add(&this->_value, arg, to_gcc_memory_order(order));
#endif /* defined(STDROMANO_WIN) */
    }

    T fetch_sub(T arg,
                STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4)
        {
            return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&this->_value), -arg);
        }
        else if constexpr (sizeof(T) == 8)
        {
            return _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(&this->_value), -arg);
        }
#elif defined(STDROMANO_LINUX)
        return __atomic_fetch_sub(&this->_value, arg, to_gcc_memory_order(order));
#endif /* defined(STDROMANO_WIN) */
    }

    bool compare_exchange(T& expected,
                          T value,
                          STDROMANO_MAYBE_UNUSED MemoryOrder success = MemoryOrder::SeqCst,
                          STDROMANO_MAYBE_UNUSED MemoryOrder failure = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if constexpr (sizeof(T) == 4) 
        {
            T orig = static_cast<T>(_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&_value),
                                                                value,
                                                                expected));
            bool ok = (orig == expected);

            if(!ok)
            {
                expected = orig;
            }

            return ok;
        } 
        else if constexpr (sizeof(T) == 8) 
        {
            T orig = static_cast<T>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(&_value),
                                                                  value,
                                                                  expected));
            bool ok = (orig == expected);

            if(!ok) 
            {
                expected = orig;
            }

            return ok;
        }
#elif defined(STDROMANO_LINUX)
        return __atomic_compare_exchange_n(&value,
                                           &expected,
                                           value,
                                           false,
                                           to_gcc_memory_order(success),
                                           to_gcc_memory_order(failure));
#endif /* defined(STDROMANO_WIN) */
    }

    T operator++() noexcept 
    {
        return fetch_add(1) + 1;
    }

    T operator++(int) noexcept 
    {
        return fetch_add(1);
    }

    T operator--() noexcept 
    {
        return fetch_sub(1) - 1;
    }

    T operator--(int) noexcept 
    {
        return fetch_sub(1);
    }
};

template<>
class Atomic<bool> 
{
private:
    volatile long _value;

public:
    Atomic() noexcept : _value(0) {}
    constexpr Atomic(bool value) noexcept : _value(value ? 1 : 0) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    bool load(MemoryOrder order = MemoryOrder::SeqCst) const noexcept 
    {
#if defined(STDROMANO_WIN)
        long val = _InterlockedOr(const_cast<volatile long*>(&this->_value), 0);

        if(order == MemoryOrder::Acquire)
        {
            MemoryBarrier();
        }

        return val != 0;
#elif defined(STDROMANO_LINUX)
        int v = __atomic_load_n(&this->_value, to_gcc_memory_order(order));
        return v != 0;
#endif
    }

    void store(bool value, MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        if(order == MemoryOrder::Release) 
        {
            MemoryBarrier();
        }

        _InterlockedExchange(const_cast<volatile long*>(&this->_value), value ? 1 : 0);
#elif defined(STDROMANO_LINUX)
        __atomic_store_n(&this->_value, value ? 1 : 0, to_gcc_memory_order(order));
#endif
    }

    bool exchange(bool value,
                  STDROMANO_MAYBE_UNUSED MemoryOrder order = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        long old = _InterlockedExchange(const_cast<volatile long*>(&this->_value), value ? 1 : 0);
        return old != 0;
#elif defined(STDROMANO_LINUX)
        int old = __atomic_exchange_n(&this->_value, value ? 1 : 0, to_gcc_memory_order(order));
        return old != 0;
#endif
    }

    bool compare_exchange(bool& expected, bool value,
                          STDROMANO_MAYBE_UNUSED MemoryOrder success = MemoryOrder::SeqCst,
                          STDROMANO_MAYBE_UNUSED MemoryOrder failure = MemoryOrder::SeqCst) noexcept 
    {
#if defined(STDROMANO_WIN)
        long expected_val = expected ? 1 : 0;
        long value_val = value ? 1 : 0;
        long old = _InterlockedCompareExchange(const_cast<volatile long*>(&this->_value), value_val, expected_val);
        bool success_flag = (old == expected_val);

        if(!success_flag)
        {
            expected = (old != 0);
        }

        return success_flag;
#elif defined(STDROMANO_LINUX)
        int expected_val = expected ? 1 : 0;
        int value_val = value ? 1 : 0;
        bool success_flag = __atomic_compare_exchange_n(
            &this->_value, &expected_val, value_val, false,
            to_gcc_memory_order(success), to_gcc_memory_order(failure));
        if (!success_flag) expected = (expected_val != 0);
        return success_flag;
#endif
    }
};

#if defined(STDROMANO_WIN)
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API Atomic<std::size_t>;
STDROMANO_EXPIMP_TEMPLATE template class STDROMANO_API Atomic<std::uint32_t>;
#endif /* defined(STDROMANO_WIN) */

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_ATOMIC) */