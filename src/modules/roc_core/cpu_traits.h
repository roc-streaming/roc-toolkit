/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/cpu_traits.h
//! @brief CPU traits.

#ifndef ROC_CORE_CPU_TRAITS_H_
#define ROC_CORE_CPU_TRAITS_H_

#include "roc_core/stddefs.h"

// On some environments, some standard macros like UINT_MAX or SIZE_MAX are
// defined to expressions that can't be evaluated at preprocessing time and a
// simple check like SIZE_MAX == 0xffffffffffffffff may not always work. Thus,
// we do our best and check many different standard and non-standard macros.
//
// We avoid comparisons with other macros like SIZE_MAX == UINT_MAX, since if
// both are defined to something that can not be evaluated by preprocessor,
// both will be assumed to be zeros and the check may incorrectly pass.

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
//! Whether we target 64-bit CPUs.
#define ROC_CPU_64BIT 0
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
