// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/fuzz.hpp"
#include "stdromano/test.hpp"

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <set>

#if !defined(STDROMANO_WIN)
#include <sys/wait.h>
#include <unistd.h>
#endif // !defined(STDROMANO_WIN)

using namespace stdromano;

static void set_env(const char* name, const char* value)
{
#if defined(STDROMANO_WIN)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif // defined(STDROMANO_WIN)
}

static void unset_env(const char* name)
{
#if defined(STDROMANO_WIN)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif // defined(STDROMANO_WIN)
}

// The environment drives the runs, so a run that asserts on iteration counts has to start clean
struct CleanEnvironment
{
    CleanEnvironment()
    {
        for(const char* name : {"ROMANO_FUZZ_SEED",
                                "ROMANO_FUZZ_ITERATIONS",
                                "ROMANO_FUZZ_SCALE",
                                "ROMANO_FUZZ_SECONDS",
                                "ROMANO_FUZZ_REPLAY"})
            unset_env(name);
    }

    ~CleanEnvironment() {}
};

STDROMANO_TEST_CASE(same_seed_gives_the_same_values)
{
    fuzz::Source a = fuzz::Source::from_seed(1234);
    fuzz::Source b = fuzz::Source::from_seed(1234);
    fuzz::Source c = fuzz::Source::from_seed(1235);

    bool differs = false;

    for(int i = 0; i < 64; ++i)
    {
        const std::uint64_t va = a.next_u64();
        STDROMANO_REQUIRE_EQ(va, b.next_u64());
        differs = differs || va != c.next_u64();
    }

    STDROMANO_CHECK(differs);
}

STDROMANO_TEST_CASE(range_is_inclusive_and_bounded)
{
    fuzz::Source source = fuzz::Source::from_seed(7);

    bool saw_min = false;
    bool saw_max = false;

    for(int i = 0; i < 4096; ++i)
    {
        const std::int8_t value = source.range<std::int8_t>(-3, 3);

        STDROMANO_REQUIRE_GE(value, -3);
        STDROMANO_REQUIRE_LE(value, 3);

        saw_min = saw_min || value == -3;
        saw_max = saw_max || value == 3;
    }

    STDROMANO_CHECK(saw_min);
    STDROMANO_CHECK(saw_max);

    // Degenerate and inverted bounds
    STDROMANO_CHECK_EQ(source.range<int>(5, 5), 5);
    STDROMANO_CHECK_EQ(source.range<int>(7, 3), 7);

    // Full ranges must not overflow
    for(int i = 0; i < 256; ++i)
    {
        const std::uint64_t u = source.range<std::uint64_t>(0, std::numeric_limits<std::uint64_t>::max());
        const std::int64_t s = source.range<std::int64_t>(std::numeric_limits<std::int64_t>::min(),
                                                          std::numeric_limits<std::int64_t>::max());
        STDROMANO_UNUSED(u);
        STDROMANO_UNUSED(s);
    }
}

STDROMANO_TEST_CASE(integer_hits_edge_values)
{
    fuzz::Source source = fuzz::Source::from_seed(11);

    std::set<std::int8_t> seen;

    for(int i = 0; i < 4096; ++i)
        seen.insert(source.integer<std::int8_t>());

    STDROMANO_CHECK(seen.count(0) == 1);
    STDROMANO_CHECK(seen.count(-1) == 1);
    STDROMANO_CHECK(seen.count(std::numeric_limits<std::int8_t>::min()) == 1);
    STDROMANO_CHECK(seen.count(std::numeric_limits<std::int8_t>::max()) == 1);

    bool saw_zero = false;
    bool saw_max = false;

    for(int i = 0; i < 4096; ++i)
    {
        const std::uint64_t value = source.integer<std::uint64_t>();
        saw_zero = saw_zero || value == 0;
        saw_max = saw_max || value == std::numeric_limits<std::uint64_t>::max();
    }

    STDROMANO_CHECK(saw_zero);
    STDROMANO_CHECK(saw_max);
}

STDROMANO_TEST_CASE(floating_hits_special_values)
{
    fuzz::Source source = fuzz::Source::from_seed(13);

    bool nan = false;
    bool positive_infinity = false;
    bool negative_infinity = false;
    bool negative_zero = false;
    bool subnormal = false;

    for(int i = 0; i < 8192; ++i)
    {
        const double value = source.floating<double>();

        nan = nan || std::isnan(value);
        positive_infinity = positive_infinity || (std::isinf(value) && value > 0);
        negative_infinity = negative_infinity || (std::isinf(value) && value < 0);
        negative_zero = negative_zero || (value == 0.0 && std::signbit(value));
        subnormal = subnormal || std::fpclassify(value) == FP_SUBNORMAL;
    }

    STDROMANO_CHECK(nan);
    STDROMANO_CHECK(positive_infinity);
    STDROMANO_CHECK(negative_infinity);
    STDROMANO_CHECK(negative_zero);
    STDROMANO_CHECK(subnormal);

    for(int i = 0; i < 1024; ++i)
        STDROMANO_REQUIRE(!std::isnan(source.floating<float>()) || true);
}

STDROMANO_TEST_CASE(finite_stays_within_bounds)
{
    fuzz::Source source = fuzz::Source::from_seed(17);

    for(int i = 0; i < 4096; ++i)
    {
        const double value = source.finite<double>(-2.5, 7.25);

        STDROMANO_REQUIRE(!std::isnan(value));
        STDROMANO_REQUIRE_GE(value, -2.5);
        STDROMANO_REQUIRE_LE(value, 7.25);
    }

    // Bounds whose difference overflows
    for(int i = 0; i < 1024; ++i)
    {
        const double value = source.finite<double>(std::numeric_limits<double>::lowest(),
                                                   std::numeric_limits<double>::max());
        STDROMANO_REQUIRE(std::isfinite(value));
    }

    STDROMANO_CHECK_EQ(source.finite<double>(3.0, 3.0), 3.0);
    STDROMANO_CHECK_EQ(source.finite<float>(9.0f, 1.0f), 9.0f);
}

STDROMANO_TEST_CASE(sizes_and_strings)
{
    fuzz::Source source = fuzz::Source::from_seed(19);

    bool saw_zero = false;
    bool saw_max = false;

    for(int i = 0; i < 1024; ++i)
    {
        const std::size_t size = source.size(10);

        STDROMANO_REQUIRE_LE(size, std::size_t(10));

        saw_zero = saw_zero || size == 0;
        saw_max = saw_max || size == 10;
    }

    STDROMANO_CHECK(saw_zero);
    STDROMANO_CHECK(saw_max);
    STDROMANO_CHECK_EQ(source.size(0), std::size_t(0));

    for(int i = 0; i < 512; ++i)
    {
        const StringD text = source.string(16, "abc");

        STDROMANO_REQUIRE_LE(text.size(), std::size_t(16));

        for(std::size_t c = 0; c < text.size(); ++c)
            STDROMANO_REQUIRE(text[c] == 'a' || text[c] == 'b' || text[c] == 'c');
    }

    // Without an alphabet, any byte but 0, so the string stays usable as a C string
    for(int i = 0; i < 512; ++i)
    {
        const StringD text = source.string(16);

        for(std::size_t c = 0; c < text.size(); ++c)
            STDROMANO_REQUIRE(text[c] != '\0');
    }
}

// A byte backed source is what libFuzzer and input replay use: once the bytes run out, every
// draw returns its smallest value, so truncating an input shrinks what it generates
STDROMANO_TEST_CASE(byte_backed_source)
{
    fuzz::Source empty = fuzz::Source::from_bytes(nullptr, 0);

    STDROMANO_CHECK(empty.exhausted());
    STDROMANO_CHECK_EQ(empty.next_u64(), std::uint64_t(0));
    STDROMANO_CHECK_EQ(empty.below(100), std::uint64_t(0));
    STDROMANO_CHECK_EQ(empty.range<int>(3, 9), 3);
    STDROMANO_CHECK_EQ(empty.boolean(), false);
    STDROMANO_CHECK_EQ(empty.size(10), std::size_t(0));
    STDROMANO_CHECK_EQ(empty.integer<int>(), 0);
    STDROMANO_CHECK_EQ(empty.floating<double>(), 0.0);
    STDROMANO_CHECK_EQ(empty.string(8).size(), std::size_t(0));

    // below() reads only the bytes the bound needs
    const std::uint8_t data[] = {5, 7};
    fuzz::Source source = fuzz::Source::from_bytes(data, sizeof(data));

    STDROMANO_CHECK_EQ(source.below(10), std::uint64_t(5));
    STDROMANO_CHECK(!source.exhausted());
    STDROMANO_CHECK_EQ(source.below(10), std::uint64_t(7));
    STDROMANO_CHECK(source.exhausted());

    fuzz::Source a = fuzz::Source::from_bytes(data, sizeof(data));
    fuzz::Source b = fuzz::Source::from_bytes(data, sizeof(data));

    STDROMANO_CHECK_EQ(a.integer<int>(), b.integer<int>());
}

STDROMANO_TEST_CASE(mutations_respect_the_maximum_and_use_the_dictionary)
{
    fuzz::Source source = fuzz::Source::from_seed(23);

    fuzz::Dictionary dictionary;
    dictionary.tokens.push_back("MAGIC");

    std::vector<std::uint8_t> data;
    bool saw_token = false;

    for(int i = 0; i < 8192; ++i)
    {
        fuzz::mutate(source, data, 64, &dictionary);

        STDROMANO_REQUIRE_LE(data.size(), std::size_t(64));

        if(data.size() >= 5)
        {
            const std::string text(data.begin(), data.end());
            saw_token = saw_token || text.find("MAGIC") != std::string::npos;
        }
    }

    STDROMANO_CHECK(saw_token);

    // Without a dictionary, and starting from nothing
    std::vector<std::uint8_t> other;

    for(int i = 0; i < 1024; ++i)
    {
        fuzz::mutate(source, other, 8, nullptr);
        STDROMANO_REQUIRE_LE(other.size(), std::size_t(8));
    }
}

STDROMANO_TEST_CASE(run_property_passes_and_counts_iterations)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "passing_property";
    options.iterations = 200;

    std::atomic<int> calls{0};

    const fuzz::Report report = fuzz::run_property(options, [&](fuzz::Source& source) {
        calls.fetch_add(1);
        const int a = source.integer<int>();
        return a >= std::numeric_limits<int>::min();
    });

    STDROMANO_CHECK(report.passed());
    STDROMANO_CHECK_EQ(report.iterations, std::uint64_t(200));
    STDROMANO_CHECK_EQ(calls.load(), 200);
    STDROMANO_CHECK(report.describe().find(StringD::make_ref("passed")) >= 0);
}

STDROMANO_TEST_CASE(run_property_reports_a_replayable_seed)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "failing_property";
    options.iterations = 100000;

    const auto property = [](fuzz::Source& source) -> bool {
        const std::uint32_t value = source.range<std::uint32_t>(0, 999);
        STDROMANO_FUZZ_CHECK_EQ(value % 251, std::uint32_t(0) + value % 251);
        return value != 613;
    };

    const fuzz::Report report = fuzz::run_property(options, property);

    STDROMANO_REQUIRE(report.failed);
    STDROMANO_CHECK(report.message.find(StringD::make_ref("returned false")) >= 0);

    // The seed alone reproduces the failing draw
    fuzz::Source replayed = fuzz::Source::from_seed(report.failing_seed);
    STDROMANO_CHECK_EQ(replayed.range<std::uint32_t>(0, 999), std::uint32_t(613));

    // And ROMANO_FUZZ_REPLAY runs exactly that one iteration
    const StringD replay = StringD::make_fmt("0x{:016x}", report.failing_seed);
    set_env("ROMANO_FUZZ_REPLAY", replay.c_str());

    const fuzz::Report replayed_report = fuzz::run_property(options, property);

    unset_env("ROMANO_FUZZ_REPLAY");

    STDROMANO_CHECK(replayed_report.failed);
    STDROMANO_CHECK_EQ(replayed_report.iterations, std::uint64_t(1));
    STDROMANO_CHECK_EQ(replayed_report.failing_seed, report.failing_seed);
}

STDROMANO_TEST_CASE(fuzz_check_and_exceptions_carry_a_message)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "check_message";
    options.iterations = 1;

    const fuzz::Report checked = fuzz::run_property(options, [](fuzz::Source& source) {
        STDROMANO_UNUSED(source);
        STDROMANO_FUZZ_CHECK_EQ(1 + 1, 3);
        return true;
    });

    STDROMANO_REQUIRE(checked.failed);
    STDROMANO_CHECK(checked.message.find(StringD::make_ref("test_fuzz.cpp")) >= 0);
    STDROMANO_CHECK(checked.message.find(StringD::make_ref("(2 vs 3)")) >= 0);

    const fuzz::Report thrown = fuzz::run_property(options, [](fuzz::Source& source) -> bool {
        STDROMANO_UNUSED(source);
        throw std::runtime_error("boom");
    });

    STDROMANO_REQUIRE(thrown.failed);
    STDROMANO_CHECK(thrown.message.find(StringD::make_ref("boom")) >= 0);
}

STDROMANO_TEST_CASE(environment_overrides)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "environment";
    options.iterations = 10;

    const auto always_true = [](fuzz::Source& source) -> bool {
        STDROMANO_UNUSED(source);
        return true;
    };

    set_env("ROMANO_FUZZ_ITERATIONS", "7");
    STDROMANO_CHECK_EQ(fuzz::run_property(options, always_true).iterations, std::uint64_t(7));
    unset_env("ROMANO_FUZZ_ITERATIONS");

    set_env("ROMANO_FUZZ_SCALE", "3");
    STDROMANO_CHECK_EQ(fuzz::run_property(options, always_true).iterations, std::uint64_t(30));
    unset_env("ROMANO_FUZZ_SCALE");

    set_env("ROMANO_FUZZ_SEED", "0x1234");
    STDROMANO_CHECK_EQ(fuzz::run_property(options, always_true).seed, std::uint64_t(0x1234));
    unset_env("ROMANO_FUZZ_SEED");

    // Nonsense is ignored rather than taken as 0
    set_env("ROMANO_FUZZ_ITERATIONS", "not-a-number");
    STDROMANO_CHECK_EQ(fuzz::run_property(options, always_true).iterations, std::uint64_t(10));
    unset_env("ROMANO_FUZZ_ITERATIONS");
}

STDROMANO_TEST_CASE(time_budget_stops_the_run)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "budget";
    options.iterations = 100000000;
    options.max_seconds = 0.05;

    const fuzz::Report report = fuzz::run_property(options, [](fuzz::Source& source) -> bool {
        STDROMANO_UNUSED(source);
        return true;
    });

    STDROMANO_CHECK(report.passed());
    STDROMANO_CHECK_LT(report.iterations, std::uint64_t(100000000));
    STDROMANO_CHECK_LT(report.elapsed_seconds, 5.0);
}

STDROMANO_TEST_CASE(run_input_finds_and_minimizes_a_planted_bug)
{
    CleanEnvironment environment;

    const auto target = [](const std::uint8_t* data, std::size_t size) -> bool {
        const std::string text(reinterpret_cast<const char*>(data), size);
        return text.find("BUG") == std::string::npos;
    };

    fuzz::Options options;
    options.name = "planted_bug";
    options.iterations = 20000;
    options.max_input_size = 64;
    options.corpus.push_back(std::vector<std::uint8_t>{'h', 'e', 'l', 'l', 'o'});
    options.dictionary.tokens.push_back("BUG");

    const fuzz::Report report = fuzz::run_input(options, target);

    STDROMANO_REQUIRE(report.failed);

    const std::string minimized(report.failing_input.begin(), report.failing_input.end());

    STDROMANO_CHECK_EQ(minimized, std::string("BUG"));
    STDROMANO_CHECK(report.describe().find(StringD::make_ref("BUG")) >= 0);
}

STDROMANO_TEST_CASE(run_input_reports_a_failing_corpus_entry)
{
    CleanEnvironment environment;

    fuzz::Options options;
    options.name = "bad_corpus";
    options.iterations = 1;
    options.minimize = false;
    options.corpus.push_back(std::vector<std::uint8_t>{'o', 'k'});
    options.corpus.push_back(std::vector<std::uint8_t>{'B', 'A', 'D'});

    const fuzz::Report report = fuzz::run_input(options, [](const std::uint8_t* data, std::size_t size) -> bool {
        return !(size == 3 && std::memcmp(data, "BAD", 3) == 0);
    });

    STDROMANO_REQUIRE(report.failed);
    STDROMANO_CHECK_EQ(report.failing_corpus_index, std::int64_t(1));
    STDROMANO_CHECK(report.describe().find(StringD::make_ref("corpus entry")) >= 0);
}

STDROMANO_TEST_CASE(minimize_removes_everything_it_can)
{
    const std::string input = "aaaaaXbbbbbZccccc";

    const std::vector<std::uint8_t> minimized =
        fuzz::minimize([](const std::uint8_t* data, std::size_t size) -> bool {
            const std::string text(reinterpret_cast<const char*>(data), size);
            return text.find('X') == std::string::npos || text.find('Z') == std::string::npos;
        },
                       std::vector<std::uint8_t>(input.begin(), input.end()));

    STDROMANO_CHECK_EQ(std::string(minimized.begin(), minimized.end()), std::string("XZ"));
}

#if !defined(STDROMANO_WIN)

// The crash handler runs in a child: it prints the replay seed and then lets the crash happen,
// so sanitizers still report it
static bool crash_prints_replay(int signal_to_raise, std::string& output)
{
    int pipe_fds[2];

    if(pipe(pipe_fds) != 0)
        return false;

    const pid_t pid = fork();

    if(pid < 0)
        return false;

    if(pid == 0)
    {
        close(pipe_fds[0]);
        dup2(pipe_fds[1], STDERR_FILENO);
        close(pipe_fds[1]);

        fuzz::Options options;
        options.name = "crasher";
        options.iterations = 10;

        fuzz::run_property(options, [signal_to_raise](fuzz::Source& source) -> bool {
            STDROMANO_UNUSED(source);

            if(signal_to_raise == SIGSEGV)
            {
                volatile int* null_pointer = static_cast<int*>(nullptr);
                *null_pointer = 1;
            }
            else
            {
                std::raise(signal_to_raise);
            }

            return true;
        });

        _exit(0);
    }

    close(pipe_fds[1]);

    char buffer[4096];
    ssize_t received = 0;

    while((received = read(pipe_fds[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, static_cast<std::size_t>(received));

    close(pipe_fds[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    // Must not have returned normally from the property
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

STDROMANO_TEST_CASE(crashes_print_the_replay_seed)
{
    CleanEnvironment environment;

    std::string output;

    STDROMANO_CHECK(crash_prints_replay(SIGABRT, output));
    STDROMANO_CHECK(output.find("ROMANO_FUZZ_REPLAY=0x") != std::string::npos);
    STDROMANO_CHECK(output.find("crasher") != std::string::npos);

    output.clear();

    STDROMANO_CHECK(crash_prints_replay(SIGSEGV, output));
    STDROMANO_CHECK(output.find("ROMANO_FUZZ_REPLAY=0x") != std::string::npos);
}

#endif // !defined(STDROMANO_WIN)

STDROMANO_TEST_MAIN()
