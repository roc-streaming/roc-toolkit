/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_peer/context.h"
#include "roc_peer/receiver.h"

namespace roc {
namespace peer {

TEST_GROUP(receiver) {
    ContextConfig context_config;
    pipeline::ReceiverConfig receiver_config;
};

TEST(receiver, source) {
    Context context(context_config);
    CHECK(context.valid());

    Receiver receiver(context, receiver_config);
    CHECK(receiver.valid());

    UNSIGNED_LONGS_EQUAL(receiver.source().sample_rate(),
                         receiver_config.common.output_sample_rate);
}

TEST(receiver, bind) {
    Context context(context_config);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig port_config;
        port_config.protocol = pipeline::Proto_RTP;
        CHECK(port_config.address.set_host_port_ipv4("127.0.0.1", 0));
        CHECK(port_config.address.port() == 0);

        CHECK(receiver.bind(pipeline::Port_AudioSource, port_config));
        CHECK(port_config.address.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);
}

TEST(receiver, bad_config) {
    context_config.max_frame_size = 1;

    Context context(context_config);
    CHECK(context.valid());

    Receiver receiver(context, receiver_config);
    CHECK(!receiver.valid());
}

} // namespace peer
} // namespace roc
