// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_TRAITS)
#define __STDROMANO_LINALG_TRAITS

#include "stdromano/stdromano.hpp"

#include <type_traits>

STDROMANO_NAMESPACE_BEGIN

/* Detect if the type is compatible with the DenseMatrix */

template<typename T>
struct is_compatible { static constexpr bool value = false; };

template<> struct is_compatible<float> { static constexpr bool value = true; };
template<> struct is_compatible<double> { static constexpr bool value = true; };
template<> struct is_compatible<std::int32_t> { static constexpr bool value = true; };

template<typename T>
constexpr bool is_compatible_v = is_compatible<T>::value;

/* Convert a type to the string extension of a cl kernel */

template<typename T>
struct type_to_cl_kernel_ext {};

template<> struct type_to_cl_kernel_ext<float> { static constexpr const char* value = "f"; };
template<> struct type_to_cl_kernel_ext<double> { static constexpr const char* value = "d"; };
template<> struct type_to_cl_kernel_ext<std::int32_t> { static constexpr const char* value = "i"; };

template<typename T>
constexpr const char* type_to_cl_kernel_ext_v = type_to_cl_kernel_ext<T>::value;

/* Constants */

template<typename T>
struct make_zero { static constexpr T value = static_cast<T>(0); };

template<typename T>
constexpr T make_zero_v = make_zero<T>::value;

template<typename T>
struct make_one { static constexpr T value = static_cast<T>(1); };

template<typename T>
constexpr T make_one_v = make_one<T>::value;

/* Vector helpers */

template<class, class = std::void_t<>>
struct has_xy : std::false_type {};

template<class T>
struct has_xy<T, std::void_t<
    decltype(std::declval<T>().x),
    decltype(std::declval<T>().y)
>> : std::true_type {};

template<class T>
constexpr bool has_xy_v = has_xy<T>::value;

template<class, class = std::void_t<>>
struct has_xyz : std::false_type {};

template<class T>
struct has_xyz<T, std::void_t<
    decltype(std::declval<T>().x),
    decltype(std::declval<T>().y),
    decltype(std::declval<T>().z)
>> : std::true_type {};

template<class T>
constexpr bool has_xyz_v = has_xyz<T>::value;

template<class, class = std::void_t<>>
struct has_xyzw : std::false_type {};

template<class T>
struct has_xyzw<T, std::void_t<
    decltype(std::declval<T>().x),
    decltype(std::declval<T>().y),
    decltype(std::declval<T>().z),
    decltype(std::declval<T>().w)
>> : std::true_type {};

template<class T>
constexpr bool has_xyzw_v = has_xyzw<T>::value;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_TRAITS) */
