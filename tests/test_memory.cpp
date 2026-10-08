// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/memory.hpp"

#include "fixtures.hpp"

#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
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

namespace {

struct Throwing
{
    Throwing()
    {
        throw std::runtime_error("Throwing constructor");
    }
};

struct alignas(64) OverAligned
{
    float values[16];
};

class Base
{
public:
    static inline std::int64_t live = 0;

    int base_value = 1;

    Base()
    {
        ++Base::live;
    }

    virtual ~Base()
    {
        --Base::live;
    }
};

class Padding
{
public:
    std::uint64_t padding[3] = {0xA, 0xB, 0xC};

    virtual ~Padding() = default;
};

// Base is not the first base, so the Base* is not the start of the allocation
class Derived : public Padding, public Base
{
public:
    Tracked tracked{"derived"};
};

class Counted
{
public:
    static inline std::atomic<std::int64_t> destroyed{0};

    std::uint64_t value = 0xDEADBEEF;

    ~Counted()
    {
        Counted::destroyed.fetch_add(1);
    }
};

template <bool ThreadSafe>
void check_shared_copy_and_move()
{
    const std::int64_t live = Tracked::live();

    {
        SharedPtr<Tracked, ThreadSafe> a = detail::make_shared<Tracked, ThreadSafe>("shared");
        STDROMANO_REQUIRE(a);
        STDROMANO_CHECK_EQ(a->value(), std::string("shared"));
        STDROMANO_CHECK_EQ(a.use_count(), 1u);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

        SharedPtr<Tracked, ThreadSafe> b = a;
        STDROMANO_CHECK_EQ(a.use_count(), 2u);
        STDROMANO_CHECK(a == b);

        SharedPtr<Tracked, ThreadSafe> c = std::move(b);
        STDROMANO_CHECK(!b);
        STDROMANO_CHECK(b == nullptr);
        STDROMANO_CHECK_EQ(a.use_count(), 2u);

        SharedPtr<Tracked, ThreadSafe> d;
        d = c;
        STDROMANO_CHECK_EQ(a.use_count(), 3u);

        const SharedPtr<Tracked, ThreadSafe>& self = d;
        d = self;
        STDROMANO_CHECK_EQ(a.use_count(), 3u);

        d = nullptr;
        STDROMANO_CHECK_EQ(a.use_count(), 2u);

        c.reset();
        STDROMANO_CHECK_EQ(a.use_count(), 1u);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

        SharedPtr<Tracked, ThreadSafe> e = detail::make_shared<Tracked, ThreadSafe>("other");
        a.swap(e);
        STDROMANO_CHECK_EQ(a->value(), std::string("other"));
        STDROMANO_CHECK_EQ(e->value(), std::string("shared"));
        STDROMANO_CHECK(a != e);

        e = a;
        STDROMANO_CHECK_EQ(Tracked::live(), live + 1);
        STDROMANO_CHECK_EQ(a.use_count(), 2u);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

template <bool ThreadSafe>
void check_weak()
{
    const std::int64_t live = Tracked::live();

    WeakPtr<Tracked, ThreadSafe> empty;
    STDROMANO_CHECK(empty.expired());
    STDROMANO_CHECK(!empty.lock());

    SharedPtr<Tracked, ThreadSafe> shared = detail::make_shared<Tracked, ThreadSafe>("weak");
    WeakPtr<Tracked, ThreadSafe> weak = shared;
    WeakPtr<Tracked, ThreadSafe> weak_copy = weak;

    STDROMANO_CHECK(!weak.expired());
    STDROMANO_CHECK_EQ(weak.use_count(), 1u);

    {
        SharedPtr<Tracked, ThreadSafe> locked = weak.lock();
        STDROMANO_REQUIRE(locked);
        STDROMANO_CHECK(locked == shared);
        STDROMANO_CHECK_EQ(shared.use_count(), 2u);
    }

    STDROMANO_CHECK_EQ(shared.use_count(), 1u);

    shared.reset();

    // The object dies with the last strong reference, the block lives until the last weak one
    STDROMANO_CHECK_EQ(Tracked::live(), live);
    STDROMANO_CHECK(weak.expired());
    STDROMANO_CHECK(weak_copy.expired());
    STDROMANO_CHECK(!weak.lock());

    weak.reset();
    STDROMANO_CHECK(weak.expired());
}

template <bool ThreadSafe>
void check_overaligned()
{
    for(int i = 0; i < 64; ++i)
    {
        SharedPtr<OverAligned, ThreadSafe> ptr = detail::make_shared<OverAligned, ThreadSafe>();
        STDROMANO_CHECK_EQ(reinterpret_cast<std::uintptr_t>(ptr.get()) % 64, 0u);
        std::memset(ptr->values, 0xFF, sizeof(ptr->values));

        WeakPtr<OverAligned, ThreadSafe> weak = ptr;
        STDROMANO_CHECK(weak.lock().get() == ptr.get());
    }
}

} // namespace

STDROMANO_TEST_CASE(smart_pointers_are_pointer_sized)
{
    STDROMANO_CHECK_EQ(sizeof(UniquePtr<int>), sizeof(void*));
    STDROMANO_CHECK_EQ(sizeof(SharedPtr<int>), sizeof(void*));
    STDROMANO_CHECK_EQ(sizeof(SharedPtrST<int>), sizeof(void*));
    STDROMANO_CHECK_EQ(sizeof(WeakPtr<int>), sizeof(void*));
    STDROMANO_CHECK_EQ(sizeof(WeakPtrST<int>), sizeof(void*));
}

STDROMANO_TEST_CASE(unique_ptr_owns_and_destroys)
{
    const std::int64_t live = Tracked::live();

    {
        UniquePtr<Tracked> ptr = make_unique<Tracked>("unique");
        STDROMANO_REQUIRE(ptr);
        STDROMANO_CHECK_EQ(ptr->value(), std::string("unique"));
        STDROMANO_CHECK_EQ((*ptr).value(), std::string("unique"));
        STDROMANO_CHECK_EQ(Tracked::live(), live + 1);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);

    UniquePtr<Tracked> empty;
    STDROMANO_CHECK(!empty);
    STDROMANO_CHECK(empty == nullptr);
    empty.reset();
    STDROMANO_CHECK(!empty);
}

STDROMANO_TEST_CASE(unique_ptr_move_release_and_reset)
{
    const std::int64_t live = Tracked::live();

    UniquePtr<Tracked> a = make_unique<Tracked>("a");
    Tracked* raw = a.get();

    UniquePtr<Tracked> b = std::move(a);
    STDROMANO_CHECK(!a);
    STDROMANO_CHECK(b.get() == raw);

    a = make_unique<Tracked>("other");
    STDROMANO_CHECK_EQ(Tracked::live(), live + 2);

    a = std::move(b);
    STDROMANO_CHECK(!b);
    STDROMANO_CHECK(a.get() == raw);
    STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

    a.swap(b);
    STDROMANO_CHECK(!a);
    STDROMANO_CHECK(b.get() == raw);

    Tracked* released = b.release();
    STDROMANO_CHECK(!b);
    STDROMANO_CHECK(released == raw);
    STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

    UniquePtr<Tracked> adopted(released);
    adopted.reset(make_unique<Tracked>("reset").release());
    STDROMANO_CHECK_EQ(adopted->value(), std::string("reset"));
    STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

    adopted = nullptr;
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(unique_ptr_upcast_frees_the_whole_object)
{
    const std::int64_t live = Tracked::live();
    const std::int64_t base_live = Base::live;

    {
        UniquePtr<Derived> derived = make_unique<Derived>();
        Base* as_base = derived.get();
        STDROMANO_CHECK(static_cast<void*>(as_base) != static_cast<void*>(derived.get()));

        UniquePtr<Base> base = std::move(derived);
        STDROMANO_CHECK(!derived);
        STDROMANO_CHECK(base.get() == as_base);
        STDROMANO_CHECK_EQ(base->base_value, 1);
        STDROMANO_CHECK_EQ(Tracked::live(), live + 1);

        UniquePtr<Base> other;
        other = make_unique<Derived>();
        STDROMANO_CHECK_EQ(Base::live, base_live + 2);
    }

    STDROMANO_CHECK_EQ(Base::live, base_live);
    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(smart_pointers_free_memory_when_the_constructor_throws)
{
    STDROMANO_CHECK_THROWS(make_unique<Throwing>(), std::runtime_error);
    STDROMANO_CHECK_THROWS(make_shared<Throwing>(), std::runtime_error);
    STDROMANO_CHECK_THROWS(make_shared_st<Throwing>(), std::runtime_error);
}

STDROMANO_TEST_CASE(smart_pointers_respect_overalignment)
{
    for(int i = 0; i < 64; ++i)
    {
        UniquePtr<OverAligned> ptr = make_unique<OverAligned>();
        STDROMANO_CHECK_EQ(reinterpret_cast<std::uintptr_t>(ptr.get()) % 64, 0u);
    }

    check_overaligned<true>();
    check_overaligned<false>();
}

STDROMANO_TEST_CASE(shared_ptr_mt_copy_and_move)
{
    check_shared_copy_and_move<true>();
}

STDROMANO_TEST_CASE(shared_ptr_st_copy_and_move)
{
    check_shared_copy_and_move<false>();
}

STDROMANO_TEST_CASE(weak_ptr_mt_lock_and_expire)
{
    check_weak<true>();
}

STDROMANO_TEST_CASE(weak_ptr_st_lock_and_expire)
{
    check_weak<false>();
}

STDROMANO_TEST_CASE(shared_ptr_of_const)
{
    const std::int64_t live = Tracked::live();

    {
        SharedPtr<const Tracked> ptr = make_shared<const Tracked>("const");
        WeakPtr<const Tracked> weak = ptr;
        STDROMANO_CHECK_EQ(weak.lock()->value(), std::string("const"));

        UniquePtr<const Tracked> unique = make_unique<const Tracked>("const unique");
        STDROMANO_CHECK_EQ(unique->value(), std::string("const unique"));
        STDROMANO_CHECK_EQ(Tracked::live(), live + 2);
    }

    STDROMANO_CHECK_EQ(Tracked::live(), live);
}

STDROMANO_TEST_CASE(shared_ptr_mt_concurrent_copies_destroy_once)
{
    constexpr std::size_t NUM_THREADS = 8;
    constexpr std::size_t NUM_ITERATIONS = 20000;

    for(std::size_t round = 0; round < 16; ++round)
    {
        const std::int64_t destroyed = Counted::destroyed.load();

        SharedPtr<Counted> shared = make_shared<Counted>();
        WeakPtr<Counted> weak = shared;

        std::atomic<bool> corrupted{false};
        std::vector<std::thread> threads;

        for(std::size_t i = 0; i < NUM_THREADS; ++i)
        {
            threads.emplace_back([shared, weak, &corrupted, NUM_ITERATIONS]() mutable {
                for(std::size_t j = 0; j < NUM_ITERATIONS; ++j)
                {
                    SharedPtr<Counted> copy = shared;
                    WeakPtr<Counted> weak_copy = copy;
                    SharedPtr<Counted> locked = weak_copy.lock();

                    if(locked == nullptr || locked->value != 0xDEADBEEF)
                        corrupted.store(true);
                }

                // Each thread races to drop its references, one of them drops the last one
                shared.reset();
                weak.reset();
            });
        }

        shared.reset();

        for(std::thread& thread : threads)
            thread.join();

        STDROMANO_CHECK(!corrupted.load());
        STDROMANO_CHECK(weak.expired());
        STDROMANO_CHECK_EQ(Counted::destroyed.load(), destroyed + 1);
    }
}

STDROMANO_TEST_CASE(weak_ptr_mt_lock_races_with_last_release)
{
    constexpr std::size_t NUM_THREADS = 4;

    for(std::size_t round = 0; round < 256; ++round)
    {
        const std::int64_t destroyed = Counted::destroyed.load();

        SharedPtr<Counted> shared = make_shared<Counted>();
        WeakPtr<Counted> weak = shared;

        std::atomic<bool> start{false};
        std::atomic<bool> corrupted{false};
        std::vector<std::thread> threads;

        for(std::size_t i = 0; i < NUM_THREADS; ++i)
        {
            threads.emplace_back([weak, &start, &corrupted]() {
                while(!start.load())
                {
                }

                // lock() either fails or returns an object that stays alive while we hold it
                while(SharedPtr<Counted> locked = weak.lock())
                {
                    if(locked->value != 0xDEADBEEF)
                        corrupted.store(true);
                }
            });
        }

        start.store(true);
        shared.reset();

        for(std::thread& thread : threads)
            thread.join();

        STDROMANO_CHECK(!corrupted.load());
        STDROMANO_CHECK(weak.expired());
        STDROMANO_CHECK_EQ(Counted::destroyed.load(), destroyed + 1);
    }
}

STDROMANO_TEST_MAIN()
