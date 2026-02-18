/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_windows/roc_core/cpu_instructions.h
//! @brief CPU-specific instructions.

#ifndef ROC_CORE_CPU_INSTRUCTIONS_H_
#define ROC_CORE_CPU_INSTRUCTIONS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! CPU pause instruction.
//! @remarks
//!  Doesn't include any memory barriers, so the caller is responsible to
//!  insert them into the loop. Allowed to expand to nothing.
inline void cpu_relax() {
    YieldProcessor();
}

} // namespace core
} // namespace roc

#endif // ROC_CORE_CPU_INSTRUCTIONS_H_
