/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/memory_ops.h
//! @brief Memory operations like poisoning, canary values, etc.

#ifndef ROC_CORE_MEMORY_OPS_H_
#define ROC_CORE_MEMORY_OPS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory operations.
class MemoryOps {
public:
    //! Poison memory that is going to be used.
    //! Helps catching uninitialized access bugs.
    static void poison_before_use(void* data, size_t size);

    //! Poison memory that is no more used.
    //! Helps catching use after free bugs.
    static void poison_after_use(void* data, size_t size);

    //! Prepare canary memory.
    //! Helps catching buffer overflow/underflow bugs.
    static void prepare_canary(void* data, size_t size);

    //! Check canary memory.
    //! @returns true if passed.
    static bool check_canary(void* data, size_t size);

    //! Some good filler patterns for memory.
    //! If we fill memory with these values and interpret it as 16-bit or 32-bit
    //! integers, or as floats, the values will be rather high and will sound
    //! loudly when trying to play them on sound card.
    //! We use a few different patterns to make it easy to distinguish between
    //! them in debugger.
    enum {
        Pattern_BeforeUse = 0x7a,
        Pattern_AfterUse = 0x7d,
        Pattern_Canary = 0x7b,
    };
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MEMORY_OPS_H_
