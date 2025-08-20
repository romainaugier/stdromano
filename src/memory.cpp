// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#include "stdromano/memory.hpp"

#include "jemalloc/jemalloc.h"

#include <algorithm>

STDROMANO_NAMESPACE_BEGIN

DETAIL_NAMESPACE_BEGIN

void* mem_alloc(const size_t size) noexcept
{
    return je_malloc(size);
}

void* mem_calloc(const size_t count, const size_t size) noexcept
{
    return je_calloc(count, size);
}

void* mem_realloc(void* ptr, const size_t size) noexcept
{
    return je_realloc(ptr, size);
}

void* mem_crealloc(void* ptr, const size_t size) noexcept
{
    void* new_ptr = je_realloc(ptr, size);

    if(new_ptr != nullptr)
    {
        std::memset(new_ptr, 0, size);
    }

    return new_ptr;
}

void mem_free(void* ptr) noexcept
{
    je_free(ptr);
}

void* mem_aligned_alloc(const size_t size, const size_t alignment) noexcept
{
    const size_t correct_size = (size + (alignment - 1)) & ~(alignment - 1);
    return je_aligned_alloc(alignment, correct_size);
}

void mem_aligned_free(void* ptr) noexcept
{
    je_free(ptr);
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

void Arena::grow() noexcept
{
    Block* next_block;

    if(this->_current_block->_next != nullptr)
    {
        next_block = this->_current_block->_next;
    }
    else
    {
        next_block = Arena::allocate_block(this->_block_size);
        this->_capacity += this->_block_size;
    }

    next_block->_prev = this->_current_block;
    this->_current_block->_next = next_block;
    this->_current_block = next_block;
}

void Arena::clear() noexcept
{
    Destructor* destructor = this->_destructors;

    while(destructor != nullptr)
    {
        destructor->destroy_func(destructor->object_ptr);
        destructor = destructor->next;
    }

    this->_destructors = nullptr;

    Block* current = this->_current_block;

    while(current != nullptr)
    {
        current->_offset = 0;

        if(current != nullptr)
        {
            this->_current_block = current;
        }

        current = current->_prev;
    }
}

Arena::~Arena()
{
    this->clear();

    Block* current = this->_current_block;

    while(current != nullptr)
    {
        Block* tmp = current;
        current = tmp->_prev;
        mem_free(tmp);
    }
}

STDROMANO_NAMESPACE_END