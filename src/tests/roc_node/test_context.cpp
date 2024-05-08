/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_node/context.h"
#include "roc_node/receiver.h"
#include "roc_node/sender.h"

namespace roc {
namespace node {

namespace {

core::HeapArena arena;

} // namespace

TEST_GROUP(context) {};

TEST(context, reference_counting) {
    ContextConfig context_config;
    Context context(context_config, arena);

    LONGS_EQUAL(status::StatusOK, context.init_status());
    CHECK(context.getref() == 0);

    {
        pipeline::SenderSinkConfig sender_config;
        Sender sender(context, sender_config);

        CHECK(context.getref() == 1);

        {
            pipeline::ReceiverSourceConfig receiver_config;
            Receiver receiver(context, receiver_config);

            CHECK(context.getref() == 2);
        }

        CHECK(context.getref() == 1);
    }

    CHECK(context.getref() == 0);
}

} // namespace node
} // namespace roc
