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

// NOTE:
//
//  On some environments, some standard macros like UINT_MAX or SIZE_MAX are
//  defined to expressions that can't be evaluated at preprocessing time and a
//  simple check like SIZE_MAX == 0xffffffffffffffff may not always work. Thus,
//  we do our best and check many different standard and non-standard macros.
//
//  We avoid comparisons with other macros like SIZE_MAX == UINT_MAX, since if
//  both are defined to something that can not be evaluated by preprocessor,
//  both will be assumed to be zeros and the check may incorrectly pass.

#include "roc_core/cpu_defs.h"
#include "roc_core/stddefs.h"

// CPU family.

#ifndef ROC_CPU_FAMILY
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_X86_64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)     \
    || defined(_M_IX86) || defined(_X86_)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_X86
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_PPC64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) || defined(_M_PPC)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_PPC
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__s390x__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_S390X
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__s390__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_S390
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__loongarch64) || (defined(__loongarch__) && __loongarch_grlen == 64)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_LOONGARCH64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__loongarch32) || (defined(__loongarch__) && __loongarch_grlen == 32)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_LOONGARCH32
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__AARCH64EL__)                  \
    || defined(__AARCH64EB__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_AARCH64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || defined(__ARMEL__)      \
    || defined(__ARMEB__) || defined(__THUMBEL__) || defined(__THUMBEB__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_ARM
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__mips64) || defined(__mips64__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_MIPS64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__mips__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)    \
    || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_MIPS
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 64)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_RISCV64
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen == 32)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_RISCV32
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__microblaze__) || defined(__MICROBLAZE__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_MICROBLAZE
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__arc__) || defined(__ARC__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_ARC
#endif
#endif

#ifndef ROC_CPU_FAMILY
#if defined(__csky__) || defined(__CSKY__)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_CSKY
#endif
#endif

#ifndef ROC_CPU_FAMILY
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_GENERIC
#endif

// CPU endianness.

#ifndef ROC_CPU_ENDIAN
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)                             \
    && defined(__ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)
#if __BYTE_ORDER == __BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(_BYTE_ORDER) && defined(_BIG_ENDIAN) && defined(_LITTLE_ENDIAN)
#if _BYTE_ORDER == _BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#if BYTE_ORDER == BIG_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#elif BYTE_ORDER == LITTLE_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__)                 \
    || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)                 \
    || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390X || ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_BE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86_64 || ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86      \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH64                                      \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH32
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif
#endif

#ifndef ROC_CPU_ENDIAN
#error "can't detect CPU endianness"
#endif

// CPU bitness.

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
#if defined(__LP64__)
#define ROC_CPU_BITS 64
#endif
#endif

#ifndef ROC_CPU_BITS
#if defined(__LP32__) || defined(__ILP32__)
#define ROC_CPU_BITS 32
#endif
#endif

#ifndef ROC_CPU_BITS
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86_64 || ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC64    \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390X                                            \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH64                                      \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_AARCH64                                          \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_MIPS64                                           \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_RISCV64
#define ROC_CPU_BITS 64
#endif
#endif

#ifndef ROC_CPU_BITS
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86 || ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC         \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390                                             \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH32                                      \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_ARM || ROC_CPU_FAMILY == ROC_CPU_FAMILY_MIPS     \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_RISCV32
#define ROC_CPU_BITS 32
#endif
#endif

#ifndef ROC_CPU_BITS
#error "can't detect CPU bitness"
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
