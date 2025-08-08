// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - Present Romain Augier
// All rights reserved.

#pragma once

#if !defined(__STDROMANO_LINALG_TRAITS)
#define __STDROMANO_LINALG_TRAITS

#include "stdromano/stdromano.hpp"

STDROMANO_NAMESPACE_BEGIN

template<typename T>
struct is_compatible { static constexpr bool value = false; };

template<> struct is_compatible<float> { static constexpr bool value = true; };
template<> struct is_compatible<double> { static constexpr bool value = true; };
template<> struct is_compatible<std::int32_t> { static constexpr bool value = true; };

template<typename T>
constexpr bool is_compatible_v = is_compatible<T>::value;

template<typename T>
struct type_to_cl_kernel_ext {};

template<> struct type_to_cl_kernel_ext<float> { static constexpr const char* value = "f"; };
template<> struct type_to_cl_kernel_ext<double> { static constexpr const char* value = "d"; };
template<> struct type_to_cl_kernel_ext<std::int32_t> { static constexpr const char* value = "i"; };

template<typename T>
constexpr const char* type_to_cl_kernel_ext_v = type_to_cl_kernel_ext<T>::value;

STDROMANO_NAMESPACE_END

#endif /* !defined(__STDROMANO_LINALG_TRAITS) */
