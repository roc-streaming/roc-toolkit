/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_LIBRARY_TEST_HELPERS_CONTEXT_H_
#define ROC_LIBRARY_TEST_HELPERS_CONTEXT_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include "roc/config.h"
#include "roc/context.h"

namespace roc {
namespace library {
namespace test {

class Context : public core::NonCopyable<> {
public:
    Context()
        : ctx_(NULL) {
        roc_context_config config;
        memset(&config, 0, sizeof(config));

        CHECK(roc_context_open(&config, &ctx_) == 0);
        CHECK(ctx_);
    }

    ~Context() {
        CHECK(roc_context_close(ctx_) == 0);
    }

    roc_context* get() {
        return ctx_;
    }

private:
    roc_context* ctx_;
};

} // namespace test
} // namespace library
} // namespace roc

#endif // ROC_LIBRARY_TEST_HELPERS_CONTEXT_H_
