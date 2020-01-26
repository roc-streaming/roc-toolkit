/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_peer/context.h"
#include "roc_peer/receiver.h"
#include "roc_peer/sender.h"

namespace roc {
namespace peer {

namespace {

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(context) {};

TEST(context, reference_counting) {
    ContextConfig context_config;
    Context context(context_config, allocator);

    CHECK(context.valid());
    CHECK(!context.is_used());

    {
        pipeline::SenderConfig sender_config;
        Sender sender(context, sender_config);

        CHECK(context.is_used());
    }

    CHECK(!context.is_used());

    {
        pipeline::ReceiverConfig receiver_config;
        Receiver receiver(context, receiver_config);

        CHECK(context.is_used());
    }

    CHECK(!context.is_used());
}

} // namespace peer
} // namespace roc
