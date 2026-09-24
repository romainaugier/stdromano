// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/memory.hpp"

#include "mimalloc.h"

#include <algorithm>

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

void* mem_alloc(const size_t size) noexcept
{
    return mi_malloc(size);
}

void* mem_calloc(const size_t count, const size_t size) noexcept
{
    return mi_calloc(count, size);
}

void* mem_realloc(void* ptr, const size_t size) noexcept
{
    return mi_realloc(ptr, size);
}

void* mem_crealloc(void* ptr, const size_t size) noexcept
{
    void* new_ptr = mi_realloc(ptr, size);

    if(new_ptr != nullptr)
    {
        std::memset(new_ptr, 0, size);
    }

    return new_ptr;
}

void mem_free(void* ptr) noexcept
{
    mi_free(ptr);
}

void* mem_aligned_alloc(const size_t size, const size_t alignment) noexcept
{
    const size_t correct_size = (size + (alignment - 1)) & ~(alignment - 1);
    return mi_malloc_aligned(correct_size, alignment);
}

void mem_aligned_free(void* ptr) noexcept
{
    mi_free(ptr);
}

DETAIL_NAMESPACE_END

constexpr const char* units[4] = { "Bytes", "Gb", "Mb", "Kb" };

void format_byte_size(float size, char* buffer) noexcept
{
    size_t unit = 0;

    if(size > 1000000000)
    {
        unit = 1;
        size = size / 1000000000;
    }
    else if(size > 1000000)
    {
        unit = 2;
        size = size / 1000000;
    }
    else if(size > 1e3)
    {
        unit = 3;
        size = size / 1000;
    }

    std::snprintf(buffer, 16, "%.02f %s", size, units[unit]);
}

Arena::Arena(const std::size_t initial_size,
             const std::size_t block_size)
{
    this->_current_block = Arena::allocate_block(initial_size);
    this->_capacity = initial_size;
    this->_block_size = block_size;
}

Arena::Block* Arena::allocate_block(const std::size_t size) noexcept
{
    const std::size_t total_size = size + sizeof(Block);

    void* addr = mem_alloc(total_size);

    if(addr == nullptr)
    {
        return nullptr;
    }

    void* block_addr = static_cast<char*>(addr) + sizeof(Block);

    ::new(addr) Arena::Block(block_addr, size);

    return static_cast<Block*>(addr);
}

Arena::Block* Arena::first_block() const noexcept
{
    Block* current = this->_current_block;

    while(current != nullptr && current->_prev != nullptr)
        current = current->_prev;

    return current;
}

void Arena::grow(const std::size_t min_size) noexcept
{
    Block* next_block = this->_current_block->_next;

    if(next_block == nullptr || next_block->_size < min_size)
    {
        const std::size_t size = std::max(this->_block_size, min_size);

        Block* new_block = Arena::allocate_block(size);
        this->_capacity += size;

        new_block->_next = next_block;

        if(next_block != nullptr)
            next_block->_prev = new_block;

        next_block = new_block;
    }

    next_block->_offset = 0;
    next_block->_prev = this->_current_block;
    this->_current_block->_next = next_block;
    this->_current_block = next_block;
}

void Arena::clear() noexcept
{
    Destructor* destructor = this->_destructors;

    while(destructor != nullptr)
    {
        Destructor* next = destructor->next;
        destructor->destroy_func(destructor->object_ptr);
        destructor = next;
    }

    this->_destructors = nullptr;

    Block* current = this->first_block();

    this->_current_block = current;

    while(current != nullptr)
    {
        current->_offset = 0;
        current = current->_next;
    }
}

Arena::~Arena()
{
    this->clear();

    Block* current = this->first_block();

    while(current != nullptr)
    {
        Block* next = current->_next;
        mem_free(current);
        current = next;
    }
}

STDROMANO_NAMESPACE_END
