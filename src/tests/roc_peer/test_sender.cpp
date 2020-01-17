/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_fec/codec_map.h"
#include "roc_peer/context.h"
#include "roc_peer/sender.h"

namespace roc {
namespace peer {

TEST_GROUP(sender) {
    fec::CodecMap codec_map;
    ContextConfig context_config;
    pipeline::SenderConfig sender_config;
};

TEST(sender, no_sink) {
    Context context(context_config);
    CHECK(context.valid());

    Sender sender(context, sender_config);
    CHECK(sender.valid());

    CHECK(!sender.sink());
}

TEST(sender, bind_connect_sink) {
    Context context(context_config);
    CHECK(context.valid());

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);

    {
        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(local_addr.port() == 0);

        CHECK(sender.bind(local_addr));
        CHECK(local_addr.port() != 0);

        UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 1);

        pipeline::PortConfig remote_port;
        remote_port.protocol = address::EndProto_RTP;
        CHECK(remote_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));

        CHECK(sender.connect(address::EndType_AudioSource, remote_port));

        UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 1);

        CHECK(sender.sink());

        UNSIGNED_LONGS_EQUAL(sender.sink()->sample_rate(),
                             sender_config.input_sample_rate);
    }

    UNSIGNED_LONGS_EQUAL(context.event_loop().num_ports(), 0);
}

TEST(sender, port_validation) {
    Context context(context_config);
    CHECK(context.valid());

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_LDPC_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));

        // source port fec scheme mismatch
        CHECK(!sender.connect(address::EndType_AudioSource, source_port));
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_LDPC_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));

        // repair port fec scheme mismatch
        CHECK(!sender.connect(address::EndType_AudioRepair, repair_port));
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RTP;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));

        // repair port provided when fec is disabled
        CHECK(!sender.connect(address::EndType_AudioRepair, repair_port));
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioSource, source_port));

        // repair port not provided when fec is enabled
        CHECK(!sender.sink());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioRepair, repair_port));

        // source port not provided when fec is enabled
        CHECK(!sender.sink());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        // source port not provided when fec is disabled
        CHECK(!sender.sink());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioSource, source_port));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioRepair, repair_port));

        // bind was not called
        CHECK(!sender.sink());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_None;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioSource, source_port));

        // fec is disabled; everything is ok
        CHECK(sender.sink());
    }

    {
        sender_config.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;

        Sender sender(context, sender_config);
        CHECK(sender.valid());

        address::SocketAddr local_addr;
        CHECK(local_addr.set_host_port(address::Family_IPv4, "127.0.0.1", 0));
        CHECK(sender.bind(local_addr));

        pipeline::PortConfig source_port;
        source_port.protocol = address::EndProto_RTP_RS8M_Source;
        CHECK(source_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioSource, source_port));

        pipeline::PortConfig repair_port;
        repair_port.protocol = address::EndProto_RS8M_Repair;
        CHECK(repair_port.address.set_host_port(address::Family_IPv4, "127.0.0.1", 123));
        CHECK(sender.connect(address::EndType_AudioRepair, repair_port));

        if (codec_map.is_supported(packet::FEC_ReedSolomon_M8)) {
            // fec is enabled and supported; everything is ok
            CHECK(sender.sink());
        } else {
            // fec is enabled but not supported
            CHECK(!sender.sink());
        }
    }
}

} // namespace peer
} // namespace roc
