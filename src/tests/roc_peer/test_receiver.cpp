/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_fec/codec_map.h"
#include "roc_peer/context.h"
#include "roc_peer/receiver.h"

namespace roc {
namespace peer {

namespace {

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(receiver) {
    ContextConfig context_config;
    pipeline::ReceiverConfig receiver_config;
};

TEST(receiver, bad_config) {
    context_config.max_frame_size = 1;

    Context context(context_config, allocator);
    CHECK(context.valid());

    Receiver receiver(context, receiver_config);
    CHECK(!receiver.valid());
}

TEST(receiver, source) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    Receiver receiver(context, receiver_config);
    CHECK(receiver.valid());

    UNSIGNED_LONGS_EQUAL(receiver.source().sample_rate(),
                         receiver_config.common.output_sample_rate);
}

TEST(receiver, bind) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(source_port.address.port() == 0);

        CHECK(receiver.bind(address::EndType_AudioSource, source_port.protocol,
                            source_port.address));
        CHECK(source_port.address.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 1);
    }

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);
}

TEST(receiver, endpoints_no_fec) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioSource, source_port.protocol,
                            source_port.address));
    }
}

TEST(receiver, endpoints_fec) {
    Context context(context_config, allocator);
    CHECK(context.valid());

    if (!fec::CodecMap::instance().is_supported(packet::FEC_ReedSolomon_M8)) {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

        // fec is not supported
        CHECK(!receiver.bind(address::EndType_AudioSource, source_port.protocol,
                             source_port.address));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

        // fec is not supported
        CHECK(!receiver.bind(address::EndType_AudioRepair, repair_port.protocol,
                             repair_port.address));

        return;
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioSource, source_port.protocol,
                            source_port.address));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioRepair, repair_port.protocol,
                            repair_port.address));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioSource, source_port.protocol,
                            source_port.address));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_LDPC_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

        // repair port fec scheme mismatch
        CHECK(!receiver.bind(address::EndType_AudioRepair, repair_port.protocol,
                             repair_port.address));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioRepair, repair_port.protocol,
                            repair_port.address));

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_LDPC_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

        // source port fec scheme mismatch
        CHECK(!receiver.bind(address::EndType_AudioSource, source_port.protocol,
                             source_port.address));
    }

    {
        Receiver receiver(context, receiver_config);
        CHECK(receiver.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(receiver.bind(address::EndType_AudioSource, source_port.protocol,
                            source_port.address));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 0));

        // repair port provided when fec is disabled
        CHECK(!receiver.bind(address::EndType_AudioRepair, repair_port.protocol,
                             repair_port.address));
    }
}

} // namespace peer
} // namespace roc
