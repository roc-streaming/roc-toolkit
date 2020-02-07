/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TEST_HELPERS_MOCK_ALLOCATOR_H_
#define ROC_FEC_TEST_HELPERS_MOCK_ALLOCATOR_H_

#include "roc_core/heap_allocator.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace fec {
namespace test {

class MockAllocator : public core::IAllocator, public core::NonCopyable<> {
public:
    MockAllocator()
        : fail_(false) {
    }

    virtual void* allocate(size_t size) {
        if (fail_) {
            return NULL;
        }
        return ha_.allocate(size);
    }

    virtual void deallocate(void* ptr) {
        ha_.deallocate(ptr);
    }

    void set_fail(bool fail) {
        fail_ = fail;
    }

private:
    core::HeapAllocator ha_;
    bool fail_;
};

} // namespace test
} // namespace fec
} // namespace roc

#endif // ROC_FEC_TEST_HELPERS_MOCK_ALLOCATOR_H_
