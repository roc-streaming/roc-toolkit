/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_NOOP_ARENA_H_
#define ROC_PIPELINE_TEST_HELPERS_NOOP_ARENA_H_

#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace pipeline {
namespace test {

struct NoopArena : public core::IArena, public core::NonCopyable<> {
    virtual void* allocate(size_t) {
        return NULL;
    }

    virtual void deallocate(void*) {
    }
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_NOOP_ARENA_H_
