// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

// Property based testing and input fuzzing
//
// A property draws its inputs from a Source and returns whether it holds:
//
//     const auto report = stdromano::fuzz::run_property(options, [](stdromano::fuzz::Source& source) {
//         const int a = source.integer<int>();
//         const int b = source.integer<int>();
//         return my_add(a, b) == my_add(b, a);
//     });
//
// A Source is backed either by a seeded PRNG or by a byte buffer, so the same property runs with
// run_property(), replays a single seed, or runs under libFuzzer (STDROMANO_FUZZ_LIBFUZZER_*).
// An input target takes raw bytes instead, and run_input() feeds it a corpus and mutations of it,
// minimizing the input that fails.
//
// Runs are reproducible: the seed is fixed unless overridden, and every failure prints the seed
// that replays it. Environment overrides:
//
//     ROMANO_FUZZ_SEED=<n>        base seed of every run
//     ROMANO_FUZZ_ITERATIONS=<n>  iterations per run
//     ROMANO_FUZZ_SCALE=<x>       multiplies the iterations of every run
//     ROMANO_FUZZ_SECONDS=<s>     time budget per run
//     ROMANO_FUZZ_REPLAY=<seed>   runs only the iteration with this seed, as printed on failure
//
// A seed replays the draws, not what the code under test does with them: thread scheduling, or a
// per instance random seed like the one HashMap uses, can still make the same draws fail
// differently from one run to the next
//
// Runs are not reentrant: do not run two of them at the same time from different threads

#pragma once

#if !defined(__STDROMANO_FUZZ)
#define __STDROMANO_FUZZ

#include "stdromano/stdromano.hpp"
#include "stdromano/string.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

STDROMANO_NAMESPACE_BEGIN

namespace fuzz {

static constexpr std::uint64_t DEFAULT_SEED = 0x5EED0F57D20A11CEull;

class STDROMANO_API Source
{
public:
    static Source from_seed(std::uint64_t seed) noexcept;

    // Draws read the bytes in order. Once they run out, every draw returns its smallest value
    // (0, false, empty...), which is what lets a fuzzer shrink an input by truncating it
    static Source from_bytes(const std::uint8_t* data, std::size_t size) noexcept;

    std::uint64_t next_u64() noexcept;

    // Uniform in [0, bound), 0 when bound <= 1. From bytes, reads only as many as bound needs
    std::uint64_t below(std::uint64_t bound) noexcept;

    bool exhausted() const noexcept;

    bool is_byte_backed() const noexcept { return this->_bytes; }

    bool boolean() noexcept { return this->below(2) == 1; }

    bool one_in(std::uint64_t n) noexcept { return this->below(n) == 0; }

    // Uniform in [min, max], min when max <= min
    template <typename T>
    T range(T min, T max) noexcept
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "range() needs an integer type");

        using U = std::make_unsigned_t<T>;

        if(!(min < max))
            return min;

        const U span = static_cast<U>(static_cast<U>(max) - static_cast<U>(min));

        const std::uint64_t draw = static_cast<std::uint64_t>(span) == std::numeric_limits<std::uint64_t>::max()
                                       ? this->next_u64()
                                       : this->below(static_cast<std::uint64_t>(span) + 1);

        return static_cast<T>(static_cast<U>(static_cast<U>(min) + static_cast<U>(draw)));
    }

    // Any value, biased towards the ones that break code: 0, +-1, limits, powers of two +-1 and
    // small values
    template <typename T>
    T integer() noexcept
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>, "integer() needs an integer type");

        using U = std::make_unsigned_t<T>;
        using L = std::numeric_limits<T>;

        constexpr std::uint64_t bits = sizeof(T) * 8;

        switch(this->below(8))
        {
            case 0:
            {
                const T values[] = {T(0),
                                    T(1),
                                    L::min(),
                                    L::max(),
                                    static_cast<T>(L::min() + 1),
                                    static_cast<T>(L::max() - 1),
                                    static_cast<T>(std::is_signed_v<T> ? -1 : 2)};

                return values[this->below(sizeof(values) / sizeof(values[0]))];
            }
            case 1:
            {
                const U power = static_cast<U>(U(1) << this->below(bits));
                const U value = static_cast<U>(power + static_cast<U>(this->below(3)) - U(1));

                if(std::is_signed_v<T> && this->boolean())
                    return static_cast<T>(static_cast<U>(U(0) - value));

                return static_cast<T>(value);
            }
            case 2:
                if constexpr(std::is_signed_v<T>)
                    return this->range<T>(T(-16), T(16));
                else
                    return this->range<T>(T(0), T(32));
            default:
                return static_cast<T>(static_cast<U>(this->next_u64()));
        }
    }

    // Uniform in [min, max], never NaN or infinite for finite bounds
    template <typename T>
    T finite(T min, T max) noexcept
    {
        static_assert(std::is_floating_point_v<T>, "finite() needs a floating point type");

        if(!(min < max))
            return min;

        constexpr int mantissa = std::numeric_limits<T>::digits;
        constexpr std::uint64_t steps = std::uint64_t(1) << (mantissa < 53 ? mantissa : 53);

        const T unit = static_cast<T>(this->below(steps + 1)) / static_cast<T>(steps);

        // Interpolated from both ends: max - min overflows for bounds like lowest() and max()
        T value = min * (T(1) - unit) + max * unit;

        if(!(value >= min))
            value = min;

        if(!(value <= max))
            value = max;

        return value;
    }

    // Any value, biased towards the ones that break code: +-0, +-1, NaN, +-inf, subnormals,
    // limits, epsilon, then small values and arbitrary bit patterns
    template <typename T>
    T floating() noexcept
    {
        static_assert(std::is_floating_point_v<T>, "floating() needs a floating point type");

        using L = std::numeric_limits<T>;

        switch(this->below(6))
        {
            case 0:
            {
                const T values[] = {T(0),
                                    -T(0),
                                    T(1),
                                    T(-1),
                                    T(0.5),
                                    L::min(),
                                    -L::min(),
                                    L::denorm_min(),
                                    -L::denorm_min(),
                                    L::max(),
                                    L::lowest(),
                                    L::epsilon(),
                                    T(1) + L::epsilon(),
                                    L::infinity(),
                                    -L::infinity(),
                                    L::quiet_NaN()};

                return values[this->below(sizeof(values) / sizeof(values[0]))];
            }
            case 1:
                return this->finite<T>(T(-1), T(1));
            case 2:
                return this->finite<T>(T(-1e6), T(1e6));
            default:
                if constexpr(sizeof(T) == sizeof(std::uint32_t))
                {
                    const std::uint32_t bits = static_cast<std::uint32_t>(this->next_u64());
                    T value;
                    std::memcpy(&value, &bits, sizeof(value));
                    return value;
                }
                else if constexpr(sizeof(T) == sizeof(std::uint64_t))
                {
                    const std::uint64_t bits = this->next_u64();
                    T value;
                    std::memcpy(&value, &bits, sizeof(value));
                    return value;
                }
                else
                {
                    return this->finite<T>(L::lowest(), L::max());
                }
        }
    }

    // In [0, max], biased towards 0, max and small sizes
    std::size_t size(std::size_t max) noexcept;

    // In [0, count). count must not be 0
    std::size_t index(std::size_t count) noexcept { return static_cast<std::size_t>(this->below(count)); }

    template <typename T>
    const T& pick(const std::vector<T>& items) noexcept
    {
        return items[this->index(items.size())];
    }

    template <typename T>
    T pick(std::initializer_list<T> items) noexcept
    {
        return *(items.begin() + this->index(items.size()));
    }

    void fill(void* data, std::size_t size) noexcept;

    std::vector<std::uint8_t> bytes(std::size_t max_size);

    // Characters drawn from alphabet, or any byte in [1, 255] when it is null
    StringD string(std::size_t max_size, const char* alphabet = nullptr);

private:
    std::uint64_t _state[4] = {0, 0, 0, 0};
    const std::uint8_t* _data = nullptr;
    std::size_t _size = 0;
    std::size_t _position = 0;
    bool _bytes = false;
};

// Tokens the mutator splices into inputs: keywords, magic numbers, delimiters of the format
struct Dictionary
{
    std::vector<std::string> tokens;
};

// Applies one random mutation in place: bit flip, byte change, interesting byte, arithmetic,
// insertion, deletion, duplication, swap, byte run, dictionary token. Never grows past max_size
STDROMANO_API void mutate(Source& source,
                          std::vector<std::uint8_t>& data,
                          std::size_t max_size,
                          const Dictionary* dictionary = nullptr);

// Thrown by STDROMANO_FUZZ_CHECK to fail a property with a message
class PropertyFailure : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

// Returns false when the property does not hold. Throwing fails it too, with the message
using Property = std::function<bool(Source&)>;

// Returns false when the input is mishandled. Throwing fails it too, with the message
using InputTarget = std::function<bool(const std::uint8_t*, std::size_t)>;

struct Options
{
    const char* name = "fuzz";
    std::uint64_t seed = DEFAULT_SEED;
    std::uint64_t iterations = 1000;
    double max_seconds = 0.0; // 0 means no time budget

    // run_input() only
    std::size_t max_input_size = 4096;
    std::vector<std::vector<std::uint8_t>> corpus;
    Dictionary dictionary;
    bool minimize = true;

    // Prints the replay seed when the process crashes during the run, then lets the crash
    // proceed (sanitizers still report it)
    bool catch_crashes = true;
};

struct Report
{
    StringD name;
    std::uint64_t seed = 0;
    std::uint64_t iterations = 0;
    double elapsed_seconds = 0.0;

    bool failed = false;
    std::uint64_t failing_iteration = 0;
    std::uint64_t failing_seed = 0;
    StringD message;

    // run_input() only. -1 unless a corpus entry failed as is
    std::int64_t failing_corpus_index = -1;
    std::vector<std::uint8_t> failing_input;
    std::size_t original_input_size = 0;

    bool passed() const noexcept { return !this->failed; }

    StringD describe() const;
};

STDROMANO_API StringD describe(const Report& report);

inline StringD Report::describe() const { return fuzz::describe(*this); }

STDROMANO_API Report run_property(const Options& options, const Property& property);

STDROMANO_API Report run_input(const Options& options, const InputTarget& target);

// Removes chunks, then lowers bytes, as long as the target still fails. Returns the smallest
// failing input found within max_attempts calls
STDROMANO_API std::vector<std::uint8_t> minimize(const InputTarget& target,
                                                 std::vector<std::uint8_t> input,
                                                 std::size_t max_attempts = 8192);

DETAIL_NAMESPACE_BEGIN

template <typename T>
StringD display(const T& value)
{
    if constexpr(std::is_enum_v<T>)
        return StringD::make_fmt("{}", static_cast<std::underlying_type_t<T>>(value));
    else if constexpr(fmt::is_formattable<T>::value)
        return StringD::make_fmt("{}", value);
    else
        return StringD::make_from_c_str("<unprintable>");
}

DETAIL_NAMESPACE_END

} // namespace fuzz

STDROMANO_NAMESPACE_END

// Fails the enclosing property with the location and the condition
#define STDROMANO_FUZZ_CHECK(condition)                                                            \
    do                                                                                             \
    {                                                                                              \
        if(!(condition))                                                                           \
            throw stdromano::fuzz::PropertyFailure(                                                \
                stdromano::StringD::make_fmt("{}:{}: expected: {}", __FILE__, __LINE__, #condition) \
                    .c_str());                                                                     \
    } while(0)

#define STDROMANO_FUZZ_CHECK_EQ(lhs, rhs)                                                          \
    do                                                                                             \
    {                                                                                              \
        const auto& stdromano_lhs = (lhs);                                                         \
        const auto& stdromano_rhs = (rhs);                                                         \
        if(!(stdromano_lhs == stdromano_rhs))                                                      \
            throw stdromano::fuzz::PropertyFailure(                                                \
                stdromano::StringD::make_fmt("{}:{}: expected: {} == {} ({} vs {})",               \
                                             __FILE__,                                             \
                                             __LINE__,                                             \
                                             #lhs,                                                 \
                                             #rhs,                                                 \
                                             stdromano::fuzz::detail::display(stdromano_lhs),      \
                                             stdromano::fuzz::detail::display(stdromano_rhs))      \
                    .c_str());                                                                     \
    } while(0)

// libFuzzer entry points (build with -fsanitize=fuzzer): the same property or target, with the
// Source backed by libFuzzer's input
#define STDROMANO_FUZZ_LIBFUZZER_PROPERTY(property)                                                \
    extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)              \
    {                                                                                              \
        stdromano::fuzz::Source stdromano_source = stdromano::fuzz::Source::from_bytes(data, size); \
        if(!(property)(stdromano_source))                                                          \
            std::abort();                                                                          \
        return 0;                                                                                  \
    }

#define STDROMANO_FUZZ_LIBFUZZER_INPUT(target)                                                     \
    extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)              \
    {                                                                                              \
        if(!(target)(data, size))                                                                  \
            std::abort();                                                                          \
        return 0;                                                                                  \
    }

#endif // !defined(__STDROMANO_FUZZ)
