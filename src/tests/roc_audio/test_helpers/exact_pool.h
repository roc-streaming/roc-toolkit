/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_EXACT_POOL_H_
#define ROC_AUDIO_TEST_HELPERS_EXACT_POOL_H_

#include "roc_core/ipool.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

// Pool which allocates every object with a separate malloc() of exactly the requested
// size, without any padding and without poisoning, so that valgrind can detect any
// access past the end of a frame buffer or any use of uninitialized samples.
class ExactPool : public core::IPool {
public:
    explicit ExactPool(size_t object_size)
        : object_size_(object_size) {
    }

    virtual size_t allocation_size() const {
        return object_size_;
    }

    virtual size_t object_size() const {
        return object_size_;
    }

    virtual bool reserve(size_t) {
        return true;
    }

    virtual void* allocate() {
        return malloc(object_size_);
    }

    virtual void deallocate(void* memory) {
        free(memory);
    }

private:
    const size_t object_size_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_EXACT_POOL_H_
