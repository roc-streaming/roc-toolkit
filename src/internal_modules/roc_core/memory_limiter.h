/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/memory_limiter.h
//! @brief Memory limiter.

#ifndef ROC_CORE_MEMORY_LIMITER_H_
#define ROC_CORE_MEMORY_LIMITER_H_

#include "roc_core/atomic_size.h"
#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory limiter.
//! This class can be used to keep track of memory being consumed. This is done through
//! the acquire and release methods. The class is used within classes such as LimitedPool,
//! LimitedArena.
class MemoryLimiter : public NonCopyable<> {
public:
    //! Initialize memory limiter.
    //! @p max_bytes is the maximum total amount of memory that can be acquired. If 0,
    //! then there is no limit, in which case, only tracking will be performed.
    explicit MemoryLimiter(const char* name, size_t max_bytes);

    //! Destroy memory limiter.
    //! This will panic if memory is still tracked as acquired.
    ~MemoryLimiter();

    //! Track acquired memory.
    //! @returns
    //!  true if successful i.e. maximum limit not breached.
    ROC_NODISCARD bool acquire(size_t num_bytes);

    //! Track released memory.
    //! This will panic if we are releasing more than what is currently acquired.
    void release(size_t num_bytes);

    //! Get number of bytes currently acquired.
    size_t num_acquired();

private:
    const char* name_;
    const size_t max_bytes_;
    AtomicSize bytes_acquired_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MEMORY_LIMITER_H_
