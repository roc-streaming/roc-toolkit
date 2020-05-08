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

#if UINTPTR_MAX == 0xffffffffffffffff || __UINTPTR_MAX__ == 0xffffffffffffffff           \
    || SIZE_MAX == 0xffffffffffffffff || __SIZE_MAX__ == 0xffffffffffffffff              \
    || __SIZEOF_POINTER__ == 8 || __SIZEOF_SIZE_T__ == 8 || __LP64__
#define ROC_CPU_64BIT 1
#else
//! Whether we target 64-bit CPUs.
//! @remarks
//!  - On some environments, some standard macros like UINT_MAX or SIZE_MAX are
//!    defined to expressions that can't be evaluated by preprocessor and a simple
//!    check like SIZE_MAX == 0xffffffffffffffff may not always work. Thus, we
//!    do our best and check many different standard and non-standard macros.
//!  - We avoid comparisons with other macros like SIZE_MAX == UINT_MAX, since if
//!    both are defined to something that can not be evaluated by preprocessor,
//!    both will be assumed to be zeros and the check may incorrectly pass.
#define ROC_CPU_64BIT 0
#endif

#endif // ROC_CORE_CPU_TRAITS_H_
