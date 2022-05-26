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

#include "roc_core/iallocator.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Allocation policy for objects (de)allocated using IAllocator.
class StandardAllocation {
public:
    StandardAllocation()
        : allocator_(NULL) {
    }

    StandardAllocation(IAllocator& allocator)
        : allocator_(&allocator) {
    }

    //! Destroy object and deallocate its memory.
    template <class T> void destroy(T& object) {
        if (!allocator_) {
            roc_panic("allocation policy: null allocator");
        }
        allocator_->destroy_object(object);
    }

protected:
    //! Get allocator.
    IAllocator& allocator() const {
        if (!allocator_) {
            roc_panic("allocation policy: null allocator");
        }
        return *allocator_;
    }

private:
    IAllocator* allocator_;
};

//! Allocation policy for objects (de)allocated using speciailized factory.
template <class Factory> class FactoryAllocation {
public:
    FactoryAllocation()
        : factory_(NULL) {
    }

    FactoryAllocation(Factory& factory)
        : factory_(&factory) {
    }

    //! Destroy object and deallocate its memory.
    template <class T> void destroy(T& object) {
        if (!factory_) {
            roc_panic("allocation policy: null factory");
        }
        factory_->destroy(object);
    }

protected:
    //! Get factory.
    Factory& factory() const {
        if (!factory_) {
            roc_panic("allocation policy: null factory");
        }
        return *factory_;
    }

private:
    Factory* factory_;
};

//! Allocation policy for objects (de)allocated using custom functions.
class CustomAllocation {
    typedef void (*DestroyFunc)(void*);

public:
    CustomAllocation()
        : destroy_func_(NULL) {
    }

    template <class T>
    CustomAllocation(void (*destroy_func)(T*))
        : destroy_func_((DestroyFunc)destroy_func) {
    }

    //! Destroy object and deallocate its memory.
    template <class T> void destroy(T& object) {
        if (!destroy_func_) {
            roc_panic("allocation policy: null func");
        }
        destroy_func_(&object);
    }

private:
    DestroyFunc destroy_func_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ALLOCATION_POLICY_H_
