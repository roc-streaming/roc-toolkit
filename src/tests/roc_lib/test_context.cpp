/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <string.h>

#include "roc/context.h"

namespace roc {

TEST_GROUP(context){};

TEST(context, ops) {
    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_start(context));
        LONGS_EQUAL(0, roc_context_stop(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_start(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_stop(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_start(context));
        LONGS_EQUAL(-1, roc_context_start(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_stop(context));
        LONGS_EQUAL(0, roc_context_stop(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }

    {
        roc_context* context = roc_context_open(NULL);
        CHECK(context);
        LONGS_EQUAL(0, roc_context_stop(context));
        LONGS_EQUAL(-1, roc_context_start(context));
        LONGS_EQUAL(0, roc_context_close(context));
    }
}

TEST(context, bad_args) {
    LONGS_EQUAL(-1, roc_context_start(NULL));
    LONGS_EQUAL(-1, roc_context_stop(NULL));
    LONGS_EQUAL(-1, roc_context_close(NULL));
}

} // namespace roc
