/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/cpu_traits.h
//! @brief CPU traits.

//! On some environments, some standard macros like UINT_MAX or SIZE_MAX are
//! defined to expressions that can't be evaluated at preprocessing time and a
//! simple check like SIZE_MAX == 0xffffffffffffffff may not always work. Thus,
//! we do our best and check many different standard and non-standard macros.
//!
//! We avoid comparisons with other macros like SIZE_MAX == UINT_MAX, since if
//! both are defined to something that can not be evaluated by preprocessor,
//! both will be assumed to be zeros and the check may incorrectly pass.

#ifndef ROC_CORE_CPU_TRAITS_H_
#define ROC_CORE_CPU_TRAITS_H_

#include "roc_core/stddefs.h"

// Detect if we're using big-endian CPU.

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef _BYTE_ORDER
#if _BYTE_ORDER == _BIG_ENDIAN
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef BYTE_ORDER
#if BYTE_ORDER == BIG_ENDIAN
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef __BIG_ENDIAN__
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#ifdef _BIG_ENDIAN
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#if defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__)                 \
    || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__) || defined(_M_PPC)
#define ROC_CPU_BIG_ENDIAN 1
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#if defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)                 \
    || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(_M_IX86)  \
    || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM)
#define ROC_CPU_BIG_ENDIAN 0
#endif
#endif

#ifndef ROC_CPU_BIG_ENDIAN
#define ROC_CPU_BIG_ENDIAN 0
#endif

#if ROC_CPU_BIG_ENDIAN
#define ROC_CPU_LITTLE_ENDIAN 0
#else
#define ROC_CPU_LITTLE_ENDIAN 1
#endif

// Detect if we're using 64-bit CPU.

#ifndef ROC_CPU_64BIT
#ifdef UINTPTR_MAX
#if UINTPTR_MAX == 0xffffffffffffffff
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef __UINTPTR_MAX__
#if __UINTPTR_MAX__ == 0xffffffffffffffff
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef SIZE_MAX
#if SIZE_MAX == 0xffffffffffffffff
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef __SIZE_MAX__
#if __SIZE_MAX__ == 0xffffffffffffffff
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef __SIZEOF_POINTER__
#if __SIZEOF_POINTER__ == 8
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef __SIZEOF_SIZE_T__
#if __SIZEOF_SIZE_T__ == 8
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#ifdef __LP64__
#if __LP64__
#define ROC_CPU_64BIT 1
#endif
#endif
#endif

#ifndef ROC_CPU_64BIT
#define ROC_CPU_64BIT 0
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
