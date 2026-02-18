/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/allocation_policy.h
//! @brief Allocation policies.

#ifndef ROC_CORE_ALLOCATION_POLICY_H_
#define ROC_CORE_ALLOCATION_POLICY_H_

#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for objects allocated using IArena.
//! @remarks
//!  Objects allocated on arena should inherit either ArenaAllocation (to use with
//!  ScopedPtr) or RefCounted<ArenaAllocation> (to use with SharedPtr).
class ArenaAllocation {
public:
    //! Initialize.
    explicit ArenaAllocation(IArena& arena)
        : arena_(arena) {
    }

    virtual ~ArenaAllocation();

    //! Destroy object and return memory to arena.
    //! @remarks
    //!  Usually default implementation is fine, but you may need to override
    //!  it if you're using multiple inheritance.
    virtual void dispose() {
        arena_.dispose_object(*this);
    }

protected:
    //! Get arena.
    IArena& arena() const {
        return arena_;
    }

private:
    IArena& arena_;
};

//! Base class for objects allocated using IPool.
//! @remarks
//!  Objects allocated on arena should inherit either PoolAllocation (to use with
//!  ScopedPtr) or RefCounted<PoolAllocation> (to use with SharedPtr).
class PoolAllocation {
public:
    //! Initialize.
    explicit PoolAllocation(IPool& pool)
        : pool_(pool) {
    }

    virtual ~PoolAllocation();

    //! Destroy object and return memory to pool.
    //! @remarks
    //!  Usually default implementation is fine, but you may need to override
    //!  it if you're using multiple inheritance.
    virtual void dispose() {
        pool_.dispose_object(*this);
    }

protected:
    //! Get pool.
    IPool& pool() const {
        return pool_;
    }

private:
    IPool& pool_;
};

//! Base class for objects which allocation is not managed by smart pointer.
//! @remarks
//!  Useful when you want to use RefCounted for an object to enable it's safety
//!  checks (e.g. it panic in destructor if there are active references), but
//!  don't want smart pointer to manage allocation and deallocation.
class NoopAllocation {
public:
    virtual ~NoopAllocation();

    //! No-op.
    //! When SharedPtr or ScopedPtr "destroys" object, nothing happens.
    //! The user is responsible for destroying object.
    virtual void dispose() {
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALLOCATION_POLICY_H_
