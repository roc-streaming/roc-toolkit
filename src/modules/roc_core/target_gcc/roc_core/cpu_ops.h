/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gcc/roc_core/cpu_ops.h
//! @brief CPU-specific instructions.

#ifndef ROC_CORE_CPU_OPS_H_
#define ROC_CORE_CPU_OPS_H_

namespace roc {
namespace core {

#if defined(__i386__) || defined(__x86_64__)

//! CPU pause instruction.
inline void cpu_relax() {
    __asm__ __volatile__("pause" ::: "memory");
}

#elif defined(__aarch64__)

//! CPU pause instruction.
inline void cpu_relax() {
    __asm__ __volatile__("yield" ::: "memory");
}

#else

//! CPU pause instruction.
inline void cpu_relax() {
    __asm__ __volatile__("" ::: "memory");
}

#endif

} // namespace core
} // namespace roc

#endif // ROC_CORE_CPU_OPS_H_
