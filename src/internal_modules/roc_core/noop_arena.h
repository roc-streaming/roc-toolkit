/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/noop_arena.h
//! @brief No-op arena implementation.

#ifndef ROC_CORE_NOOP_ARENA_H_
#define ROC_CORE_NOOP_ARENA_H_

#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! No-op arena implementation.
class NoopArenaImpl : public IArena, public NonCopyable<> {
public:
    //! Allocate memory no-op.
    //! @returns
    //!  Always returns null.
    virtual void* allocate(size_t size);

    //! Deallocate memory no-op.
    virtual void deallocate(void* ptr);

    //! Compute allocated size no-op.
    //! @returns
    //!  Always 0.
    virtual size_t compute_allocated_size(size_t size) const;

    //! Allocated size given pointer no-op.
    //! @returns
    //!  Always 0.
    virtual size_t allocated_size(void* ptr) const;
};

static NoopArenaImpl NoopArena;

} // namespace core
} // namespace roc

#endif // ROC_CORE_NOOP_ARENA_H_
