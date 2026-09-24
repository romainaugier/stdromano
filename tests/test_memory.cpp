// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/memory.hpp"

#include "fixtures.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace stdromano;
using fixtures::Tracked;

STDROMANO_TEST_CASE(arena_emplace_constructs_the_objects)
{
    Arena arena(4096);

    int* value = arena.emplace<int>(12);
    STDROMANO_CHECK_EQ(*value, 12);

    StringD* ref = arena.emplace<StringD>(StringD::make_ref("this is a string ref"));
    StringD* alloced = arena.emplace<StringD>(StringD("this is a string{}", " alloced"));
    StringD* emplaced = arena.emplace<StringD>("this is a string emplaced");

    STDROMANO_CHECK_EQ(std::strcmp(ref->c_str(), "this is a string ref"), 0);
    STDROMANO_CHECK_EQ(std::strcmp(alloced->c_str(), "this is a string alloced"), 0);
    STDROMANO_CHECK_EQ(std::strcmp(emplaced->c_str(), "this is a string emplaced"), 0);

    for(std::size_t i = 0; i < 10000; ++i)
        STDROMANO_REQUIRE_EQ(*arena.emplace<std::size_t>(i), i);
}

STDROMANO_TEST_CASE(arena_clear_runs_the_destructors)
{
    const std::int64_t live = Tracked::live();

    {
        Arena arena(256, 128);

        for(int i = 0; i < 1000; ++i)
            arena.emplace<Tracked>(std::to_string(i));

        STDROMANO_CHECK_EQ(Tracked::live(), live + 1000);

        arena.clear();
        STDROMANO_CHECK_EQ(Tracked::live(), live);

        for(int i = 0; i < 10; ++i)
            arena.emplace<Tracked>(std::to_string(i));
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(arena_allocate_is_contiguous_within_a_block)
{
    Arena arena(4096);

    char* data = static_cast<char*>(arena.allocate(16));
    std::memcpy(data, "Hello, World!\n", 15);

    char* next = static_cast<char*>(arena.allocate(10));

    STDROMANO_CHECK_EQ(next - data, 16);
    STDROMANO_CHECK_EQ(std::strcmp(data, "Hello, World!\n"), 0);
}

STDROMANO_TEST_CASE(arena_objects_larger_than_a_block)
{
    Arena arena(64, 64);

    struct Big
    {
        char bytes[1000];
    };

    Big* big = arena.emplace<Big>();
    std::memset(big->bytes, 0xAB, sizeof(big->bytes));

    char* raw = static_cast<char*>(arena.allocate(5000));
    std::memset(raw, 0xCD, 5000);

    STDROMANO_CHECK_EQ(static_cast<unsigned char>(big->bytes[999]), 0xABu);
}

STDROMANO_TEST_CASE(fuzz_arena_allocations_do_not_overlap)
{
    const std::int64_t live = Tracked::live();

    const auto report = fuzz::run_property(fixtures::options("arena_allocations", 300), [](fuzz::Source& source) {
        Arena arena(source.range<std::size_t>(1, 512), source.range<std::size_t>(1, 512));

        struct Allocation
        {
            unsigned char* data;
            std::size_t size;
            unsigned char pattern;
        };

        std::vector<Allocation> allocations;
        std::vector<Tracked*> objects;

        const std::size_t steps = source.range<std::size_t>(1, 200);

        for(std::size_t step = 0; step < steps; ++step)
        {
            switch(source.index(4))
            {
                case 0:
                {
                    const std::size_t size = source.range<std::size_t>(1, 700);
                    const std::size_t alignment = std::size_t(1) << source.range<std::size_t>(0, 6);

                    unsigned char* data = static_cast<unsigned char*>(arena.allocate_aligned(size, alignment));

                    STDROMANO_FUZZ_CHECK_EQ(reinterpret_cast<std::uintptr_t>(data) % alignment, 0u);

                    const unsigned char pattern = static_cast<unsigned char>(source.range<int>(0, 255));
                    std::memset(data, pattern, size);
                    allocations.push_back({data, size, pattern});
                    break;
                }
                case 1:
                {
                    const std::size_t size = source.range<std::size_t>(1, 700);
                    unsigned char* data = static_cast<unsigned char*>(arena.allocate(size));
                    const unsigned char pattern = static_cast<unsigned char>(source.range<int>(0, 255));
                    std::memset(data, pattern, size);
                    allocations.push_back({data, size, pattern});
                    break;
                }
                case 2:
                {
                    Tracked* object = arena.emplace<Tracked>(std::to_string(step));
                    STDROMANO_FUZZ_CHECK_EQ(reinterpret_cast<std::uintptr_t>(object) % alignof(Tracked), 0u);
                    objects.push_back(object);
                    break;
                }
                default:
                    if(source.one_in(10))
                    {
                        arena.clear();
                        allocations.clear();
                        objects.clear();
                    }
                    break;
            }
        }

        for(const Allocation& allocation : allocations)
            for(std::size_t i = 0; i < allocation.size; ++i)
                STDROMANO_FUZZ_CHECK_EQ(allocation.data[i], allocation.pattern);

        for(Tracked* object : objects)
            STDROMANO_FUZZ_CHECK(!object->value().empty());

        return true;
    });

    TESTS_REQUIRE_PROPERTY(report);
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(aligned_alloc_is_aligned)
{
    for(std::size_t alignment = 1; alignment <= 4096; alignment *= 2)
    {
        void* ptr = mem_aligned_alloc(100, alignment);
        STDROMANO_REQUIRE(ptr != nullptr);
        STDROMANO_CHECK_EQ(reinterpret_cast<std::uintptr_t>(ptr) % alignment, 0u);
        std::memset(ptr, 0, 100);
        mem_aligned_free(ptr);
    }
}

STDROMANO_TEST_MAIN()
