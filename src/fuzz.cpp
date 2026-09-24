// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/fuzz.hpp"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#if defined(STDROMANO_WIN)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif // !defined(WIN32_LEAN_AND_MEAN)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif // defined(STDROMANO_WIN)

STDROMANO_NAMESPACE_BEGIN

namespace fuzz {

static std::uint64_t splitmix64(std::uint64_t& state) noexcept
{
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static std::uint64_t rotl(std::uint64_t x, int k) noexcept { return (x << k) | (x >> (64 - k)); }

/* ------------------------------------------------------------------------ */
/* Source                                                                   */
/* ------------------------------------------------------------------------ */

Source Source::from_seed(std::uint64_t seed) noexcept
{
    Source source;

    std::uint64_t state = seed;

    for(auto& s : source._state)
        s = splitmix64(state);

    return source;
}

Source Source::from_bytes(const std::uint8_t* data, std::size_t size) noexcept
{
    Source source;
    source._data = data;
    source._size = data != nullptr ? size : 0;
    source._bytes = true;
    return source;
}

bool Source::exhausted() const noexcept { return this->_bytes && this->_position >= this->_size; }

std::uint64_t Source::next_u64() noexcept
{
    if(this->_bytes)
    {
        std::uint64_t value = 0;

        for(int i = 0; i < 8 && this->_position < this->_size; ++i)
            value |= static_cast<std::uint64_t>(this->_data[this->_position++]) << (8 * i);

        return value;
    }

    // xoshiro256**, one state per source: stdromano's xoshiro functions share a global one
    std::uint64_t* s = this->_state;

    const std::uint64_t result = rotl(s[1] * 5, 7) * 9;
    const std::uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);

    return result;
}

std::uint64_t Source::below(std::uint64_t bound) noexcept
{
    if(bound <= 1)
        return 0;

    if(this->_bytes)
    {
        std::uint64_t value = 0;

        for(std::uint64_t remaining = bound - 1, shift = 0; remaining != 0 && this->_position < this->_size;
            remaining >>= 8, shift += 8)
            value |= static_cast<std::uint64_t>(this->_data[this->_position++]) << shift;

        return value % bound;
    }

    // Rejection sampling: unbiased, and portable where 128 bits multiplication is not
    const std::uint64_t threshold = (0 - bound) % bound;

    while(true)
    {
        const std::uint64_t r = this->next_u64();

        if(r >= threshold)
            return r % bound;
    }
}

std::size_t Source::size(std::size_t max) noexcept
{
    switch(this->below(8))
    {
        case 0:
            return 0;
        case 1:
            return max;
        case 2:
        case 3:
        case 4:
            return static_cast<std::size_t>(this->below(std::min<std::size_t>(max, 16) + 1));
        default:
            return max == std::numeric_limits<std::size_t>::max()
                       ? static_cast<std::size_t>(this->next_u64())
                       : static_cast<std::size_t>(this->below(static_cast<std::uint64_t>(max) + 1));
    }
}

void Source::fill(void* data, std::size_t size) noexcept
{
    std::uint8_t* out = static_cast<std::uint8_t*>(data);

    for(std::size_t i = 0; i < size; i += 8)
    {
        const std::uint64_t value = this->next_u64();
        std::memcpy(out + i, &value, std::min<std::size_t>(8, size - i));
    }
}

std::vector<std::uint8_t> Source::bytes(std::size_t max_size)
{
    std::vector<std::uint8_t> out(this->size(max_size));
    this->fill(out.data(), out.size());
    return out;
}

StringD Source::string(std::size_t max_size, const char* alphabet)
{
    const std::size_t alphabet_size = alphabet != nullptr ? std::strlen(alphabet) : 0;

    std::string out(this->size(max_size), '\0');

    for(char& c : out)
        c = alphabet_size > 0 ? alphabet[this->below(alphabet_size)] : static_cast<char>(1 + this->below(255));

    return StringD::make_from_c_str(out.c_str(), out.size());
}

/* ------------------------------------------------------------------------ */
/* Mutations                                                                */
/* ------------------------------------------------------------------------ */

void mutate(Source& source, std::vector<std::uint8_t>& data, std::size_t max_size, const Dictionary* dictionary)
{
    static constexpr std::uint8_t INTERESTING[] = {0x00, 0x01, 0x7F, 0x80, 0xFF, '0', '9', 'a', 'z', ' ', '\n', '-', '.', '"'};

    const bool has_dictionary = dictionary != nullptr && !dictionary->tokens.empty();
    const std::size_t operation = source.index(has_dictionary ? 10 : 9);

    // Everything but insertions needs something to work on
    if(data.empty() && operation != 9)
    {
        data.push_back(static_cast<std::uint8_t>(source.below(256)));
    }
    else
    {
        const std::size_t position = data.empty() ? 0 : source.index(data.size());

        switch(operation)
        {
            case 0:
                data[position] ^= static_cast<std::uint8_t>(1u << source.below(8));
                break;
            case 1:
                data[position] = static_cast<std::uint8_t>(source.below(256));
                break;
            case 2:
                data[position] = INTERESTING[source.index(sizeof(INTERESTING))];
                break;
            case 3:
                data[position] = static_cast<std::uint8_t>(data[position] + source.range<int>(-8, 8));
                break;
            case 4:
            {
                const std::size_t count = 1 + source.index(8);

                for(std::size_t i = 0; i < count; ++i)
                    data.insert(data.begin() + static_cast<std::ptrdiff_t>(source.index(data.size() + 1)),
                                static_cast<std::uint8_t>(source.below(256)));
                break;
            }
            case 5:
            {
                const std::size_t count = 1 + source.index(std::min<std::size_t>(data.size() - position, 16));
                data.erase(data.begin() + static_cast<std::ptrdiff_t>(position),
                           data.begin() + static_cast<std::ptrdiff_t>(position + count));
                break;
            }
            case 6:
            {
                const std::size_t count = 1 + source.index(std::min<std::size_t>(data.size() - position, 16));
                const std::vector<std::uint8_t> chunk(data.begin() + static_cast<std::ptrdiff_t>(position),
                                                      data.begin() + static_cast<std::ptrdiff_t>(position + count));
                data.insert(data.begin() + static_cast<std::ptrdiff_t>(source.index(data.size() + 1)),
                            chunk.begin(),
                            chunk.end());
                break;
            }
            case 7:
                std::swap(data[position], data[source.index(data.size())]);
                break;
            case 8:
            {
                const std::size_t count = 1 + source.index(std::min<std::size_t>(data.size() - position, 16));
                std::fill_n(data.begin() + static_cast<std::ptrdiff_t>(position),
                            count,
                            static_cast<std::uint8_t>(source.below(256)));
                break;
            }
            case 9:
            {
                const std::string& token = source.pick(dictionary->tokens);
                const std::size_t at = source.index(data.size() + 1);

                if(source.boolean() || at + token.size() > data.size())
                    data.insert(data.begin() + static_cast<std::ptrdiff_t>(at), token.begin(), token.end());
                else
                    std::copy(token.begin(), token.end(), data.begin() + static_cast<std::ptrdiff_t>(at));
                break;
            }
            default:
                break;
        }
    }

    if(data.size() > max_size)
        data.resize(max_size);
}

/* ------------------------------------------------------------------------ */
/* Settings                                                                 */
/* ------------------------------------------------------------------------ */

struct Settings
{
    std::uint64_t seed;
    std::uint64_t iterations;
    double max_seconds;
    bool replay;
    std::uint64_t replay_seed;
};

static bool env_u64(const char* name, std::uint64_t& out) noexcept
{
    const char* value = std::getenv(name);

    if(value == nullptr || value[0] == '\0')
        return false;

    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 0);

    if(end == value || *end != '\0')
    {
        spdlog::warn("Ignoring {}=\"{}\": not an integer", name, value);
        return false;
    }

    out = static_cast<std::uint64_t>(parsed);
    return true;
}

static bool env_double(const char* name, double& out) noexcept
{
    const char* value = std::getenv(name);

    if(value == nullptr || value[0] == '\0')
        return false;

    char* end = nullptr;
    const double parsed = std::strtod(value, &end);

    if(end == value || *end != '\0' || !(parsed >= 0.0))
    {
        spdlog::warn("Ignoring {}=\"{}\": not a positive number", name, value);
        return false;
    }

    out = parsed;
    return true;
}

static Settings resolve_settings(const Options& options) noexcept
{
    Settings settings{options.seed, options.iterations, options.max_seconds, false, 0};

    env_u64("ROMANO_FUZZ_SEED", settings.seed);
    env_u64("ROMANO_FUZZ_ITERATIONS", settings.iterations);

    double scale = 1.0;

    if(env_double("ROMANO_FUZZ_SCALE", scale))
        settings.iterations = std::max<std::uint64_t>(1, static_cast<std::uint64_t>(static_cast<double>(settings.iterations) * scale));

    env_double("ROMANO_FUZZ_SECONDS", settings.max_seconds);

    settings.replay = env_u64("ROMANO_FUZZ_REPLAY", settings.replay_seed);

    return settings;
}

// Each run gets its own stream, so two properties under the same global seed do not see the same
// values. An iteration is fully described by its own seed, which is what replay needs
static std::uint64_t run_base_seed(std::uint64_t seed, const char* name) noexcept
{
    std::uint64_t hash = 0xCBF29CE484222325ull;

    for(const char* c = name != nullptr ? name : ""; *c != '\0'; ++c)
        hash = (hash ^ static_cast<std::uint8_t>(*c)) * 0x100000001B3ull;

    std::uint64_t state = seed ^ hash;
    return splitmix64(state);
}

static std::uint64_t iteration_seed(std::uint64_t base, std::uint64_t iteration) noexcept
{
    std::uint64_t state = base + iteration * 0xD1B54A32D192ED03ull;
    return splitmix64(state);
}

/* ------------------------------------------------------------------------ */
/* Crash guard                                                              */
/* ------------------------------------------------------------------------ */

// Formatted before each iteration, so the handler only has to write it
static char g_crash_message[512];
static std::size_t g_crash_message_size = 0;
static std::atomic<bool> g_guard_active{false};

static void write_crash_message() noexcept
{
#if defined(STDROMANO_WIN)
    _write(2, g_crash_message, static_cast<unsigned int>(g_crash_message_size));
#else
    const ssize_t written = write(STDERR_FILENO, g_crash_message, g_crash_message_size);
    STDROMANO_UNUSED(written);
#endif // defined(STDROMANO_WIN)
}

#if defined(STDROMANO_WIN)

static LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = nullptr;
static void (*g_previous_abort)(int) = SIG_DFL;

static LONG WINAPI crash_filter(EXCEPTION_POINTERS* info)
{
    write_crash_message();
    return g_previous_filter != nullptr ? g_previous_filter(info) : EXCEPTION_CONTINUE_SEARCH;
}

static void abort_handler(int sig)
{
    write_crash_message();
    std::signal(SIGABRT, g_previous_abort);
    std::raise(sig);
}

#else

static constexpr int CRASH_SIGNALS[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
static constexpr std::size_t NUM_CRASH_SIGNALS = sizeof(CRASH_SIGNALS) / sizeof(CRASH_SIGNALS[0]);
static struct sigaction g_previous_actions[NUM_CRASH_SIGNALS];

static void crash_handler(int sig, siginfo_t* info, void*)
{
    STDROMANO_UNUSED(info);

    write_crash_message();

    for(std::size_t i = 0; i < NUM_CRASH_SIGNALS; ++i)
    {
        if(CRASH_SIGNALS[i] == sig)
        {
            sigaction(sig, &g_previous_actions[i], nullptr);
            break;
        }
    }

    std::raise(sig);

    // Protect against a previous handler that unexpectedly returns.
    _exit(128 + sig);
}

#if defined(STDROMANO_LINUX)
// Sanitizers report UB and exit without raising a signal, so the crash handler never runs
extern "C" void __sanitizer_set_death_callback(void (*callback)()) __attribute__((weak));

static void sanitizer_death_callback()
{
    if(g_guard_active.load())
        write_crash_message();
}
#endif // defined(STDROMANO_LINUX)

#endif // defined(STDROMANO_WIN)

class CrashGuard
{
    bool _installed = false;

public:
    explicit CrashGuard(bool enabled) noexcept
    {
        bool expected = false;

        if(!enabled || !g_guard_active.compare_exchange_strong(expected, true))
            return;

#if defined(STDROMANO_WIN)
        g_previous_filter = SetUnhandledExceptionFilter(crash_filter);
        g_previous_abort = std::signal(SIGABRT, abort_handler);
#else
        struct sigaction action;
        std::memset(&action, 0, sizeof(action));
        action.sa_sigaction = crash_handler;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);

        for(std::size_t i = 0; i < NUM_CRASH_SIGNALS; ++i)
            sigaction(CRASH_SIGNALS[i], &action, &g_previous_actions[i]);

#if defined(STDROMANO_LINUX)
        if(__sanitizer_set_death_callback != nullptr)
            __sanitizer_set_death_callback(sanitizer_death_callback);
#endif // defined(STDROMANO_LINUX)
#endif // defined(STDROMANO_WIN)

        this->_installed = true;
    }

    ~CrashGuard()
    {
        if(!this->_installed)
            return;

#if defined(STDROMANO_WIN)
        SetUnhandledExceptionFilter(g_previous_filter);
        std::signal(SIGABRT, g_previous_abort);
#else
        for(std::size_t i = 0; i < NUM_CRASH_SIGNALS; ++i)
            sigaction(CRASH_SIGNALS[i], &g_previous_actions[i], nullptr);
#endif // defined(STDROMANO_WIN)

        g_guard_active.store(false);
    }

    CrashGuard(const CrashGuard&) = delete;
    CrashGuard& operator=(const CrashGuard&) = delete;

    void set(const char* name, std::uint64_t iteration, std::uint64_t seed, const char* what) noexcept
    {
        if(!this->_installed)
            return;

        const int written = std::snprintf(g_crash_message,
                                          sizeof(g_crash_message),
                                          "\n[stdromano fuzz] '%s' crashed %s %llu\n"
                                          "[stdromano fuzz] replay with ROMANO_FUZZ_REPLAY=0x%016llx\n",
                                          name,
                                          what,
                                          static_cast<unsigned long long>(iteration),
                                          static_cast<unsigned long long>(seed));

        g_crash_message_size = written < 0 ? 0 : std::min<std::size_t>(static_cast<std::size_t>(written), sizeof(g_crash_message) - 1);
    }
};

/* ------------------------------------------------------------------------ */
/* Runners                                                                  */
/* ------------------------------------------------------------------------ */

template <typename F>
static bool evaluate(F&& func, StringD& message)
{
    try
    {
        if(func())
            return true;

        message = StringD::make_from_c_str("returned false");
    }
    catch(const std::exception& e)
    {
        message = StringD::make_from_c_str(e.what());
    }
    catch(...)
    {
        // Includes the test harness' AbortCase, whose failure is already reported
        message = StringD::make_from_c_str("threw an exception of unknown type");
    }

    return false;
}

static double seconds_since(std::chrono::steady_clock::time_point start) noexcept
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

static void log_result(const Report& report)
{
    if(report.failed)
        spdlog::error("{}", describe(report));
    else
        spdlog::debug("{}", describe(report));
}

Report run_property(const Options& options, const Property& property)
{
    const Settings settings = resolve_settings(options);

    Report report;
    report.name = StringD::make_from_c_str(options.name != nullptr ? options.name : "fuzz");
    report.seed = settings.seed;

    const std::uint64_t base = run_base_seed(settings.seed, options.name);
    const auto start = std::chrono::steady_clock::now();

    CrashGuard guard(options.catch_crashes);

    const std::uint64_t iterations = settings.replay ? 1 : settings.iterations;

    for(std::uint64_t i = 0; i < iterations; ++i)
    {
        if(!settings.replay && settings.max_seconds > 0.0 && i > 0 && seconds_since(start) > settings.max_seconds)
            break;

        const std::uint64_t seed = settings.replay ? settings.replay_seed : iteration_seed(base, i);

        guard.set(report.name.c_str(), i, seed, "at iteration");

        Source source = Source::from_seed(seed);
        ++report.iterations;

        if(!evaluate([&]() -> bool { return property(source); }, report.message))
        {
            report.failed = true;
            report.failing_iteration = i;
            report.failing_seed = seed;
            break;
        }
    }

    report.elapsed_seconds = seconds_since(start);

    log_result(report);

    return report;
}

// An iteration's input only depends on its seed and the options, so replay rebuilds it exactly
static std::vector<std::uint8_t> make_input(std::uint64_t seed, const Options& options)
{
    Source source = Source::from_seed(seed);

    std::vector<std::uint8_t> input;

    if(!options.corpus.empty() && !source.one_in(8))
        input = source.pick(options.corpus);

    if(input.size() > options.max_input_size)
        input.resize(options.max_input_size);

    const std::size_t mutations = 1 + source.index(4);

    for(std::size_t m = 0; m < mutations; ++m)
        mutate(source, input, options.max_input_size, &options.dictionary);

    return input;
}

std::vector<std::uint8_t> minimize(const InputTarget& target, std::vector<std::uint8_t> input, std::size_t max_attempts)
{
    std::size_t attempts = 0;

    const auto still_fails = [&](const std::vector<std::uint8_t>& candidate) -> bool {
        ++attempts;
        StringD ignored;
        return !evaluate([&]() -> bool { return target(candidate.data(), candidate.size()); }, ignored);
    };

    bool progress = true;

    while(progress && attempts < max_attempts)
    {
        progress = false;

        for(std::size_t chunk = std::max<std::size_t>(input.size() / 2, 1); chunk >= 1 && attempts < max_attempts; chunk /= 2)
        {
            for(std::size_t begin = 0; begin + chunk <= input.size() && attempts < max_attempts;)
            {
                std::vector<std::uint8_t> candidate(input);
                candidate.erase(candidate.begin() + static_cast<std::ptrdiff_t>(begin),
                                candidate.begin() + static_cast<std::ptrdiff_t>(begin + chunk));

                if(still_fails(candidate))
                {
                    input = std::move(candidate);
                    progress = true;
                }
                else
                {
                    begin += chunk;
                }
            }

            if(chunk == 1)
                break;
        }

        for(std::size_t i = 0; i < input.size() && attempts < max_attempts; ++i)
        {
            for(const std::uint8_t lower : {std::uint8_t(0), static_cast<std::uint8_t>(input[i] / 2)})
            {
                if(lower >= input[i])
                    continue;

                std::vector<std::uint8_t> candidate(input);
                candidate[i] = lower;

                if(still_fails(candidate))
                {
                    input = std::move(candidate);
                    progress = true;
                    break;
                }
            }
        }
    }

    return input;
}

Report run_input(const Options& options, const InputTarget& target)
{
    const Settings settings = resolve_settings(options);

    Report report;
    report.name = StringD::make_from_c_str(options.name != nullptr ? options.name : "fuzz");
    report.seed = settings.seed;

    const std::uint64_t base = run_base_seed(settings.seed, options.name);
    const auto start = std::chrono::steady_clock::now();

    CrashGuard guard(options.catch_crashes);

    const auto run_one = [&](const std::vector<std::uint8_t>& input) -> bool {
        ++report.iterations;
        return evaluate([&]() -> bool { return target(input.data(), input.size()); }, report.message);
    };

    std::vector<std::uint8_t> failing;

    if(!settings.replay)
    {
        for(std::size_t k = 0; k < options.corpus.size() && !report.failed; ++k)
        {
            std::vector<std::uint8_t> input(options.corpus[k]);

            if(input.size() > options.max_input_size)
                input.resize(options.max_input_size);

            guard.set(report.name.c_str(), k, 0, "on corpus entry");

            if(!run_one(input))
            {
                report.failed = true;
                report.failing_corpus_index = static_cast<std::int64_t>(k);
                failing = std::move(input);
            }
        }
    }

    const std::uint64_t iterations = settings.replay ? 1 : settings.iterations;

    for(std::uint64_t i = 0; i < iterations && !report.failed; ++i)
    {
        if(!settings.replay && settings.max_seconds > 0.0 && i > 0 && seconds_since(start) > settings.max_seconds)
            break;

        const std::uint64_t seed = settings.replay ? settings.replay_seed : iteration_seed(base, i);

        guard.set(report.name.c_str(), i, seed, "at iteration");

        std::vector<std::uint8_t> input = make_input(seed, options);

        if(!run_one(input))
        {
            report.failed = true;
            report.failing_iteration = i;
            report.failing_seed = seed;
            failing = std::move(input);
        }
    }

    if(report.failed)
    {
        report.original_input_size = failing.size();

        if(options.minimize)
        {
            guard.set(report.name.c_str(), report.failing_iteration, report.failing_seed, "while minimizing the input of iteration");

            failing = minimize(target, std::move(failing));

            // The message of the input that is reported, not of the original one
            StringD message;

            if(!evaluate([&]() -> bool { return target(failing.data(), failing.size()); }, message))
                report.message = std::move(message);
        }

        report.failing_input = std::move(failing);
    }

    report.elapsed_seconds = seconds_since(start);

    log_result(report);

    return report;
}

/* ------------------------------------------------------------------------ */
/* Report                                                                   */
/* ------------------------------------------------------------------------ */

static StringD escape_input(const std::vector<std::uint8_t>& input, std::size_t max_bytes)
{
    StringD out;
    out.appendc("\"");

    for(std::size_t i = 0; i < input.size() && i < max_bytes; ++i)
    {
        const std::uint8_t c = input[i];

        if(c == '"' || c == '\\')
            out.appendf("\\{}", static_cast<char>(c));
        else if(c >= 0x20 && c < 0x7F)
            out.appendf("{}", static_cast<char>(c));
        else
            out.appendf("\\x{:02x}", c);
    }

    out.appendc(input.size() > max_bytes ? "\"..." : "\"");

    return out;
}

StringD describe(const Report& report)
{
    if(!report.failed)
        return StringD::make_fmt("{}: {} iteration(s) passed in {:.3f} s (seed 0x{:016x})",
                                 report.name,
                                 report.iterations,
                                 report.elapsed_seconds,
                                 report.seed);

    StringD out;

    if(report.failing_corpus_index >= 0)
        out.appendf("{}: corpus entry #{} failed: {}", report.name, report.failing_corpus_index, report.message);
    else
        out.appendf("{}: failed at iteration {}: {}\n  replay with ROMANO_FUZZ_REPLAY=0x{:016x}",
                    report.name,
                    report.failing_iteration,
                    report.message,
                    report.failing_seed);

    if(!report.failing_input.empty() || report.original_input_size > 0)
    {
        out.appendf("\n  input ({} bytes", report.failing_input.size());

        if(report.original_input_size != report.failing_input.size())
            out.appendf(", minimized from {}", report.original_input_size);

        out.appendf("): {}", escape_input(report.failing_input, 256));
    }

    return out;
}

} // namespace fuzz

STDROMANO_NAMESPACE_END
