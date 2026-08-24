// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_SPAN)
#define __STDROMANO_SPAN

#include "stdromano/stdromano.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

template <typename T>
class Span;

namespace detail
{

// Detects containers exposing data() and size()
template <typename C, typename = void>
struct has_data_and_size : std::false_type {};

template <typename C>
struct has_data_and_size<C,
                         std::void_t<decltype(std::declval<C&>().data()),
                                     decltype(std::declval<C&>().size())>> : std::true_type {};

// Element type yielded by C::data() (with references and cv stripped from the
// pointer itself but not from the pointee)
template <typename C>
using container_element_t = std::remove_pointer_t<decltype(std::declval<C&>().data())>;

template <typename C>
struct is_span : std::false_type {};

template <typename T>
struct is_span<Span<T>> : std::true_type {};

template <typename C>
struct is_std_array : std::false_type {};

template <typename U, std::size_t N>
struct is_std_array<std::array<U, N>> : std::true_type {};

// U(*)[] -> T(*)[] is convertible exactly when U and T are the same type up to added cv-qualification, 
// and it also rejects derived-to-base
template <typename U, typename T>
using is_qualification_convertible = std::is_convertible<U (*)[], T (*)[]>;

/* A container is viewable as Span<T> when it is not itself a Span (that would
   fight the copy constructor), not a raw array or std::array (handled by their
   own constructors), and its element type is qualification-convertible to T. */
template <typename C, typename T, typename = void>
struct is_viewable_container : std::false_type {};

template <typename C, typename T>
struct is_viewable_container<
    C,
    T,
    std::enable_if_t<has_data_and_size<C>::value &&
                     !is_span<std::remove_cv_t<std::remove_reference_t<C>>>::value &&
                     !is_std_array<std::remove_cv_t<std::remove_reference_t<C>>>::value &&
                     !std::is_array<std::remove_cv_t<std::remove_reference_t<C>>>::value>>
    : is_qualification_convertible<container_element_t<C>, T> {};

template <typename It>
using iterator_category_t = typename std::iterator_traits<It>::iterator_category;

template <typename It, typename = void>
struct is_random_access_iterator : std::false_type {};

template <typename It>
struct is_random_access_iterator<It, std::void_t<iterator_category_t<It>>>
    : std::is_base_of<std::random_access_iterator_tag, iterator_category_t<It>> {};

template <typename It, typename T>
using is_viewable_iterator =
    std::conjunction<is_random_access_iterator<It>,
                     is_qualification_convertible<
                         std::remove_reference_t<typename std::iterator_traits<It>::reference>,
                         T>>;

} // namespace detail

/* Non-owning view over a contiguous sequence.

   Span<T>       - mutable view
   Span<const T> - read-only view

   Two words, trivially copyable. Constness belongs to the element type, so a
   const Span<T> is a const handle to mutable elements, like T* const. */
template <typename T>
class Span
{
    T* _data;
    std::size_t _sz;

public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr Span() noexcept : _data(nullptr), _sz(0) {}

    constexpr Span(T* data, const std::size_t size) noexcept : _data(data), _sz(size) {}

    // [first, last)
    constexpr Span(T* first, T* last) noexcept : _data(first),
                                                 _sz(static_cast<std::size_t>(last - first)) {}

    // Any pair of random-access iterators over contiguous storage
    // (including the iterators used by Vector and StackVector)
    template<typename It,
             std::enable_if_t<detail::is_viewable_iterator<It, T>::value, int> = 0>
    Span(It first, It last) : _data(nullptr),
                              _sz(static_cast<std::size_t>(std::distance(first, last)))
    {
        if(this->_sz > 0)
            this->_data = std::addressof(*first);
    }

    template <typename It,
              std::enable_if_t<detail::is_viewable_iterator<It, T>::value, int> = 0>
    Span(It first, const std::size_t count) : _data(nullptr), _sz(count)
    {
        if(count > 0)
            this->_data = std::addressof(*first);
    }

    template<std::size_t N>
    constexpr Span(T (&arr)[N]) noexcept : _data(arr), _sz(N) {}

    template<typename U,
             std::size_t N,
             std::enable_if_t<detail::is_qualification_convertible<U, T>::value, int> = 0>
    constexpr Span(std::array<U, N>& arr) noexcept : _data(arr.data()), _sz(N) {}

    template<typename U,
             std::size_t N,
             std::enable_if_t<detail::is_qualification_convertible<const U, T>::value, int> = 0>
    constexpr Span(const std::array<U, N>& arr) noexcept : _data(arr.data()), _sz(N) {}

    // Vector, StackVector, std::vector, std::string, or anything else with data() and size() members
    template<typename C,
             std::enable_if_t<detail::is_viewable_container<C, T>::value, int> = 0>
    constexpr Span(C& container) noexcept : _data(container.data()), _sz(container.size()) {}

    template<typename C,
             std::enable_if_t<detail::is_viewable_container<const C, T>::value, int> = 0>
    constexpr Span(const C& container) noexcept : _data(container.data()), _sz(container.size()) {}

    // Binding a Span to a temporary container dangles immediately
    template <typename C,
              std::enable_if_t<detail::is_viewable_container<C, T>::value && !std::is_lvalue_reference<C>::value,
                               int> = 0>
    Span(C&& container) = delete;

    // Span<T> -> Span<const T>
    template<typename U,
             std::enable_if_t<!std::is_same<U, T>::value && detail::is_qualification_convertible<U, T>::value,
                              int> = 0>
    constexpr Span(const Span<U>& other) noexcept : _data(other.data()), _sz(other.size()) {}

    constexpr Span(const Span&) noexcept = default;

    constexpr Span& operator=(const Span&) noexcept = default;

    ~Span() = default;

    STDROMANO_FORCE_INLINE constexpr T* data() const noexcept
    {
        return this->_data;
    }

    STDROMANO_FORCE_INLINE constexpr std::size_t size() const noexcept
    {
        return this->_sz;
    }

    STDROMANO_FORCE_INLINE constexpr std::size_t size_bytes() const noexcept
    {
        return this->_sz * sizeof(T);
    }

    STDROMANO_FORCE_INLINE constexpr bool empty() const noexcept
    {
        return this->_sz == 0;
    }

    STDROMANO_FORCE_INLINE constexpr T& operator[](const std::size_t i) const noexcept
    {
        STDROMANO_ASSERT(i < this->_sz, "Out of bounds access");

        return this->_data[i];
    }

    STDROMANO_FORCE_INLINE constexpr T* at(const std::size_t i) const noexcept
    {
        STDROMANO_ASSERT(i < this->_sz, "Out of bounds access");

        return this->_data + i;
    }

    STDROMANO_FORCE_INLINE constexpr T& front() const noexcept
    {
        STDROMANO_ASSERT(this->_sz > 0, "front() on an empty span");

        return this->_data[0];
    }

    STDROMANO_FORCE_INLINE constexpr T& back() const noexcept
    {
        STDROMANO_ASSERT(this->_sz > 0, "back() on an empty span");

        return this->_data[this->_sz - 1];
    }

    STDROMANO_FORCE_INLINE constexpr iterator begin() const noexcept
    {
        return this->_data;
    }

    STDROMANO_FORCE_INLINE constexpr iterator end() const noexcept
    {
        return this->_data + this->_sz;
    }

    STDROMANO_FORCE_INLINE constexpr const_iterator cbegin() const noexcept
    {
        return this->_data;
    }

    STDROMANO_FORCE_INLINE constexpr const_iterator cend() const noexcept
    {
        return this->_data + this->_sz;
    }

    STDROMANO_FORCE_INLINE constexpr reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(this->end());
    }

    STDROMANO_FORCE_INLINE constexpr reverse_iterator rend() const noexcept
    {
        return reverse_iterator(this->begin());
    }

    STDROMANO_FORCE_INLINE constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(this->cend());
    }

    STDROMANO_FORCE_INLINE constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(this->cbegin());
    }

    STDROMANO_FORCE_INLINE constexpr Span first(const std::size_t count) const noexcept
    {
        STDROMANO_ASSERT(count <= this->_sz, "first(): count exceeds span size");

        return Span(this->_data, count);
    }

    STDROMANO_FORCE_INLINE constexpr Span last(const std::size_t count) const noexcept
    {
        STDROMANO_ASSERT(count <= this->_sz, "last(): count exceeds span size");

        return Span(this->_data + (this->_sz - count), count);
    }

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    constexpr Span subspan(const std::size_t offset,
                           const std::size_t count = npos) const noexcept
    {
        STDROMANO_ASSERT(offset <= this->_sz, "subspan(): offset exceeds span size");

        const std::size_t remaining = this->_sz - offset;
        const std::size_t n = (count == npos) ? remaining : count;

        STDROMANO_ASSERT(n <= remaining, "subspan(): count exceeds remaining size");

        return Span(this->_data + offset, n);
    }

    // Reinterpret the underlying bytes
    Span<std::conditional_t<std::is_const<T>::value, const unsigned char, unsigned char>> as_bytes() const noexcept
    {
        using byte_type = std::conditional_t<std::is_const<T>::value, const unsigned char, unsigned char>;

        return Span<byte_type>(reinterpret_cast<byte_type*>(this->_data), this->size_bytes());
    }
};

// Deduction guides
template <typename T>
Span(T*, std::size_t) -> Span<T>;

template <typename T>
Span(T*, T*) -> Span<T>;

template <typename It>
Span(It, It) -> Span<std::remove_reference_t<typename std::iterator_traits<It>::reference>>;

template <typename T, std::size_t N>
Span(T (&)[N]) -> Span<T>;

template <typename T, std::size_t N>
Span(std::array<T, N>&) -> Span<T>;

template <typename T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T>;

template <typename C>
Span(C&) -> Span<detail::container_element_t<C>>;

template <typename C>
Span(const C&) -> Span<const detail::container_element_t<C>>;

// Helpers for when CTAD is not available or wanting to force constness

template <typename C>
constexpr Span<detail::container_element_t<C>> make_span(C& c) noexcept
{
    return Span<detail::container_element_t<C>>(c.data(), c.size());
}

template <typename C>
constexpr Span<const detail::container_element_t<const C>> make_cspan(const C& c) noexcept
{
    return Span<const detail::container_element_t<const C>>(c.data(), c.size());
}

template <typename T>
constexpr Span<T> make_span(T* data, const std::size_t size) noexcept
{
    return Span<T>(data, size);
}

STDROMANO_NAMESPACE_END

#endif // !defined(__STDROMANO_SPAN)