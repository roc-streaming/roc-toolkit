/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_BENCH_PROFILE_H_
#define ROC_BENCH_PROFILE_H_

#include "roc_core/cpu_traits.h"

#if !defined(ROC_BENCHMARK_PROFILE_LARGE) && !defined(ROC_BENCHMARK_PROFILE_MEDIUM)      \
    && !defined(ROC_BENCHMARK_PROFILE_SMALL)

#ifndef ROC_CPU_FAMILY
#error                                                                                   \
    "ROC_CPU_FAMILY is not defined. Please define it to use ROC_BENCHMARK_PROFILE_* macros"
#endif

// LARGE profile: High-performance workstation-class architectures
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86_64 || ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC64    \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390X
#define ROC_BENCHMARK_PROFILE_LARGE
// MEDIUM profile: Powerful SBCs and capable architectures
#elif ROC_CPU_FAMILY == ROC_CPU_FAMILY_GENERIC || ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86   \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC || ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390     \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH64                                      \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_AARCH64                                          \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_MIPS64                                           \
    || ROC_CPU_FAMILY == ROC_CPU_FAMILY_RISCV64
#define ROC_BENCHMARK_PROFILE_MEDIUM
#else
// SMALL profile: Weak CPUs, embedded systems, and specialized processors
#define ROC_BENCHMARK_PROFILE_SMALL
#endif

#endif // !defined(ROC_BENCHMARK_PROFILE_...)

#endif // ROC_BENCH_PROFILE_H_