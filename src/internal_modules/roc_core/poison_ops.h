/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/poison_ops.h
//! @brief Memory poisoning.

#ifndef ROC_CORE_POISON_OPS_H_
#define ROC_CORE_POISON_OPS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory poisoning.
class PoisonOps {
public:
    //! Poison memory that is going to be used.
    //! Helps catching uninitialized access bugs.
    static void before_use(void* data, size_t size);

    //! Poison memory that is no more used.
    //! Helps catching use after free bugs.
    static void after_use(void* data, size_t size);

    //! Poison memory that is a boundary guard.
    //! Helps catching buffer overflow/underflow bugs.
    static void add_boundary_guard(void* data, size_t size);

    //! Checks memory that is a boundary guard and panics if not.
    static void check_boundary_guard(void* data, size_t size);

private:
    // Some good fillers for memory.
    // If we fill memory with these values and interpret it as 16-bit or 32-bit
    // integers, or as floats, the values will be rather high and will sound
    // loudly when trying to play them on sound card.
    // We use two different patterns to make it easy to distinguish between
    // them in debugger.
    enum {
        Pattern_BeforeUse = 0x7a,
        Pattern_AfterUse = 0x7d,
        Pattern_BoundaryGuard = 0x7b,
    };
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_POISON_OPS_H_
