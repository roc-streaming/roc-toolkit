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

//! Allocation policy for objects allocated using IArena.
class ArenaAllocation {
public:
    //! Initialize.
    ArenaAllocation(IArena& arena)
        : arena_(&arena) {
    }

    //! Destroy object and return memory to arena.
    template <class T> void destroy(T& object) {
        arena_->destroy_object(object);
    }

protected:
    //! Get arena.
    IArena& arena() const {
        return *arena_;
    }

private:
    IArena* arena_;
};

//! Allocation policy for objects allocated using IPool.
class PoolAllocation {
public:
    //! Initialize.
    PoolAllocation(IPool& pool)
        : pool_(&pool) {
    }

    //! Destroy object and return memory to pool.
    template <class T> void destroy(T& object) {
        pool_->destroy_object(object);
    }

protected:
    //! Get pool.
    IPool& pool() const {
        return *pool_;
    }

private:
    IPool* pool_;
};

//! Allocation policy for objects with custom deallocation function.
class CustomAllocation {
    typedef void (*DestroyFunc)(void*);

public:
    //! Initialize.
    template <class T>
    CustomAllocation(void (*destroy_func)(T*))
        : destroy_func_((DestroyFunc)destroy_func) {
        if (!destroy_func_) {
            roc_panic("allocation policy: null function");
        }
    }

    //! Invoke custom destruction function.
    template <class T> void destroy(T& object) {
        destroy_func_(&object);
    }

private:
    DestroyFunc destroy_func_;
};

//! Allocation policy for objects that does not have automatic deallocation.
class ManualAllocation {
public:
    //! No-op.
    //! When SharedPtr or ScopedPtr "destroys" object, nothing happens.
    //! The user is responsible for destroying it manually.
    template <class T> void destroy(T&) {
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALLOCATION_POLICY_H_
