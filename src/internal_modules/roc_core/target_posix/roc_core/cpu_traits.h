/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/cpu_traits.h
//! @brief CPU traits.

#ifndef ROC_CORE_CPU_TRAITS_H_
#define ROC_CORE_CPU_TRAITS_H_

// On some environments, some standard macros like UINT_MAX or SIZE_MAX are
// defined to expressions that can't be evaluated at preprocessing time and a
// simple check like SIZE_MAX == 0xffffffffffffffff may not always work. Thus,
// we do our best and check many different standard and non-standard macros.
//
// We avoid comparisons with other macros like SIZE_MAX == UINT_MAX, since if
// both are defined to something that can not be evaluated by preprocessor,
// both will be assumed to be zeros and the check may incorrectly pass.

#include "roc_core/stddefs.h"

//! Value of ROC_CPU_ENDIAN indicating big-endian CPU.
#define ROC_CPU_BE 1

//! Value of ROC_CPU_ENDIAN indicating little-endian CPU.
#define ROC_CPU_LE 2

// Detect CPU endianness.

#ifndef ROC_CPU_ENDIAN
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)                             \
    && defined(__ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ROC_CPU_ENDIAN ROC_CPU_BE
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)
#if __BYTE_ORDER == __BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_BE
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN)
#if _BYTE_ORDER == _BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_BE
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#if BYTE_ORDER == BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_BE
#elif BYTE_ORDER == LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__BIG_ENDIAN__)
#if __BIG_ENDIAN__ == 1
#define ROC_CPU_ENDIAN ROC_CPU_BE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__LITTLE_ENDIAN__)
#if __LITTLE_ENDIAN__ == 1
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if !defined(_BYTE_ORDER) && defined(_BIG_ENDIAN)
#if _BIG_ENDIAN == 1
#define ROC_CPU_ENDIAN ROC_CPU_BE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if !defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN)
#if _LITTLE_ENDIAN == 1
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__)                 \
    || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__) || defined(_M_PPC)
#define ROC_CPU_ENDIAN ROC_CPU_BE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)                 \
    || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(_M_IX86)  \
    || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64) || defined(_M_ARM)       \
    || defined(_X86_) || defined(__x86_64__) || defined(__i386__) || defined(__i486__)   \
    || defined(__i586__) || defined(__i686__)
#define ROC_CPU_ENDIAN ROC_CPU_LE
#endif
#endif

// Detect CPU bitness.

#ifndef ROC_CPU_BITS
#ifdef UINTPTR_MAX
#if UINTPTR_MAX == 0xffffffffffffffff
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef __UINTPTR_MAX__
#if __UINTPTR_MAX__ == 0xffffffffffffffff
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef SIZE_MAX
#if SIZE_MAX == 0xffffffffffffffff
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef __SIZE_MAX__
#if __SIZE_MAX__ == 0xffffffffffffffff
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef __SIZEOF_POINTER__
#if __SIZEOF_POINTER__ == 8
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef __SIZEOF_SIZE_T__
#if __SIZEOF_SIZE_T__ == 8
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#ifdef __LP64__
#if __LP64__
#define ROC_CPU_BITS 64
#endif
#endif
#endif

#ifndef ROC_CPU_BITS
#define ROC_CPU_BITS 32
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
