/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TEST_HELPERS_MOCK_ARENA_H_
#define ROC_FEC_TEST_HELPERS_MOCK_ARENA_H_

#include "roc_core/heap_arena.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace fec {
namespace test {

class MockArena : public core::IArena, public core::NonCopyable<> {
public:
    MockArena()
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

    virtual size_t compute_allocated_size(size_t size) const {
        return ha_.compute_allocated_size(size);
    }

    virtual size_t allocated_size(void* ptr) const {
        return ha_.allocated_size(ptr);
    }

    void set_fail(bool fail) {
        fail_ = fail;
    }

private:
    core::HeapArena ha_;
    bool fail_;
};

} // namespace test
} // namespace fec
} // namespace roc

#endif // ROC_FEC_TEST_HELPERS_MOCK_ARENA_H_
