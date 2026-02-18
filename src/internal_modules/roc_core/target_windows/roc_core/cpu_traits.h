/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_windows/roc_core/cpu_traits.h
//! @brief CPU traits.

#ifndef ROC_CORE_CPU_TRAITS_H_
#define ROC_CORE_CPU_TRAITS_H_

// NOTE:
//
//  In this version of cpu_traits.h, we check only configurations supported
//  by Windows (limited set of CPUs and always little-endian).

#include "roc_core/cpu_defs.h"
#include "roc_core/stddefs.h"

// CPU family.

#ifndef ROC_CPU_FAMILY
#if defined(_M_X64)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_X86_64
#elif defined(_M_IX86)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_X86
#elif defined(_M_ARM64)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_AARCH64
#elif defined(_M_ARM)
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_ARM
#else
#define ROC_CPU_FAMILY ROC_CPU_FAMILY_GENERIC
#endif
#endif

// CPU endianness.

#ifndef ROC_CPU_ENDIAN
#define ROC_CPU_ENDIAN ROC_CPU_ENDIAN_LE
#endif

// CPU bitness.

#ifndef ROC_CPU_BITS
#ifdef _WIN64
#define ROC_CPU_BITS 64
#else
#define ROC_CPU_BITS 32
#endif
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
