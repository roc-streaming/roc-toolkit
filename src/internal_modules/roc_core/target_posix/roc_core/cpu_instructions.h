/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/cpu_instructions.h
//! @brief CPU-specific instructions.

#ifndef ROC_CORE_CPU_INSTRUCTIONS_H_
#define ROC_CORE_CPU_INSTRUCTIONS_H_

namespace roc {
namespace core {

#ifdef __GNUC__

#if defined(__i386__) || defined(__x86_64__)

inline void cpu_relax() {
    __asm__ __volatile__("pause" ::: "memory");
}

#elif defined(__aarch64__)

inline void cpu_relax() {
    __asm__ __volatile__("yield" ::: "memory");
}

#else // unknown arch

inline void cpu_relax() {
    __asm__ __volatile__("" ::: "memory");
}

#endif

#else // !__GNUC__

//! CPU pause instruction.
//! @remarks
//!  Doesn't include any memory barriers, so the caller is responsible to
//!  insert them into the loop. Allowed to expand to nothing.
inline void cpu_relax() {
}

#endif // __GNUC__

} // namespace core
} // namespace roc

#endif // ROC_CORE_CPU_INSTRUCTIONS_H_
