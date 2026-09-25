// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

// Usage: bench_json [--budget <seconds>] [files...]
// Without files, benchmarks twitter, citm_catalog and canada from TESTS_DATA_DIR/json

#include "stdromano/json.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace stdromano;

using Clock = std::chrono::steady_clock;

struct Timing
{
    double min_ms;
    double median_ms;
    std::size_t runs;
};

// Runs func until the time budget is spent, with at least MIN_RUNS runs after one warmup run
template <typename Func>
static Timing measure(const double budget_seconds, Func&& func)
{
    static constexpr std::size_t MIN_RUNS = 5;
    static constexpr std::size_t MAX_RUNS = 100000;

    func();

    std::vector<double> samples;
    const auto budget_end = Clock::now() + std::chrono::duration<double>(budget_seconds);

    while(samples.size() < MIN_RUNS || (Clock::now() < budget_end && samples.size() < MAX_RUNS))
    {
        const auto start = Clock::now();
        func();
        samples.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
    }

    std::sort(samples.begin(), samples.end());

    return {samples.front(), samples[samples.size() / 2], samples.size()};
}

static bool read_file(const char* path, std::string& content)
{
    FILE* file = std::fopen(path, "rb");

    if(file == nullptr)
        return false;

    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::rewind(file);

    if(size <= 0)
    {
        std::fclose(file);
        return false;
    }

    content.resize(static_cast<std::size_t>(size));
    const std::size_t read = std::fread(&content[0], 1, content.size(), file);
    std::fclose(file);

    return read == content.size();
}

static const char* file_name(const char* path)
{
    const char* slash = std::strrchr(path, '/');
    const char* backslash = std::strrchr(path, '\\');
    const char* last = slash > backslash ? slash : backslash;

    return last != nullptr ? last + 1 : path;
}

static void print_row(const char* name, const char* operation, const Timing& timing, const std::size_t bytes)
{
    const double mb = static_cast<double>(bytes) / 1e6;

    std::printf("%-18s %-16s %9.3f %9.3f %9.1f %9.1f %8zu\n",
                name,
                operation,
                timing.min_ms,
                timing.median_ms,
                mb / (timing.min_ms * 1e-3),
                mb / (timing.median_ms * 1e-3),
                timing.runs);
}

// Parse -> dump -> parse -> dump must be stable, otherwise the numbers are meaningless
static bool check_round_trip(const std::string& content)
{
    Json first;

    if(!first.loads(content.data(), content.size()))
        return false;

    const StringD dumped = first.dumps();

    Json second;

    if(!second.loads(dumped.c_str(), dumped.size()))
        return false;

    return second.dumps() == dumped;
}

static bool bench_file(const char* path, const double budget_seconds, std::size_t& sink)
{
    std::string content;

    if(!read_file(path, content))
    {
        std::printf("%-18s skipped (cannot read %s)\n", file_name(path), path);
        return true;
    }

    const char* name = file_name(path);

    if(!check_round_trip(content))
    {
        std::printf("%-18s FAILED round trip\n", name);
        return false;
    }

    Json reused;

    const Timing parse_reused = measure(budget_seconds, [&] {
        sink += reused.loads(content.data(), content.size());
    });

    const Timing parse_fresh = measure(budget_seconds, [&] {
        Json json;
        sink += json.loads(content.data(), content.size());
    });

    Json document;
    document.loads(content.data(), content.size());

    const std::size_t compact_size = document.dumps(0).size();
    const std::size_t pretty_size = document.dumps(2).size();

    const Timing dump_compact = measure(budget_seconds, [&] { sink += document.dumps(0).size(); });
    const Timing dump_pretty = measure(budget_seconds, [&] { sink += document.dumps(2).size(); });

    print_row(name, "parse (reused)", parse_reused, content.size());
    print_row(name, "parse (fresh)", parse_fresh, content.size());
    print_row(name, "dump", dump_compact, compact_size);
    print_row(name, "dump (indent 2)", dump_pretty, pretty_size);

    return true;
}

int main(int argc, char** argv)
{
    double budget_seconds = 1.0;
    std::vector<std::string> paths;

    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "--budget") == 0 && i + 1 < argc)
            budget_seconds = std::atof(argv[++i]);
        else
            paths.emplace_back(argv[i]);
    }

    if(paths.empty())
        for(const char* name : {"twitter", "citm_catalog", "canada"})
            paths.push_back(std::string(TESTS_DATA_DIR) + "/json/" + name + ".json");

    std::printf("%-18s %-16s %9s %9s %9s %9s %8s\n", "file", "operation", "min ms", "med ms", "min MB/s", "med MB/s", "runs");

    std::size_t sink = 0;
    bool success = true;

    for(const std::string& path : paths)
        success &= bench_file(path.c_str(), budget_seconds, sink);

    // Keeps the measured work observable
    std::printf("(checksum %zu)\n", sink);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
