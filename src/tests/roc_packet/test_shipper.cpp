/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/shipper.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace packet {

namespace {

enum { PacketSz = 128 };

core::HeapArena arena;
PacketFactory packet_factory(arena, PacketSz);

PacketPtr new_packet() {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP | Packet::FlagPrepared);
    packet->rtp()->payload_type = rtp::PayloadType_L16_Stereo;

    core::Slice<uint8_t> buffer = packet_factory.new_packet_buffer();
    CHECK(buffer);
    packet->rtp()->payload = buffer;

    return packet;
}

class MockWriter : public IWriter, public core::NonCopyable<> {
public:
    explicit MockWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr&) {
        return code_;
    }

private:
    status::StatusCode code_;
};

struct MockComposer : public IComposer, public core::NonCopyable<> {
    MockComposer(core::IArena& arena)
        : IComposer(arena)
        , compose_call_count(0) {
    }

    virtual status::StatusCode init_status() const {
        return status::StatusOK;
    }

    virtual bool align(core::Slice<uint8_t>&, size_t, size_t) {
        return true;
    }

    virtual bool prepare(Packet&, core::Slice<uint8_t>&, size_t) {
        return true;
    }

    virtual bool pad(Packet&, size_t) {
        return true;
    }

    virtual bool compose(Packet&) {
        ++compose_call_count;
        return true;
    }

    unsigned compose_call_count;
};

} // namespace

TEST_GROUP(shipper) {};

TEST(shipper, without_address) {
    MockComposer composer(arena);
    FifoQueue queue;

    Shipper shipper(composer, queue, NULL);

    PacketPtr wp = new_packet();

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
    CHECK(wp == rp);
}

TEST(shipper, with_address) {
    address::SocketAddr address;
    CHECK(address.set_host_port_auto("127.0.0.1", 123));

    MockComposer composer(arena);
    FifoQueue queue;

    Shipper shipper(composer, queue, &address);

    PacketPtr wp = new_packet();

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK(wp->flags() & Packet::FlagUDP);
    CHECK(address == wp->udp()->dst_addr);

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
    CHECK(wp == rp);
}

TEST(shipper, packet_already_composed) {
    address::SocketAddr address;
    MockComposer composer(arena);
    FifoQueue queue;

    Shipper shipper(composer, queue, &address);

    PacketPtr wp = new_packet();
    wp->add_flags(Packet::FlagComposed);

    CHECK(wp->flags() & Packet::FlagComposed);
    LONGS_EQUAL(0, composer.compose_call_count);

    LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK(wp->flags() & Packet::FlagComposed);
    LONGS_EQUAL(0, composer.compose_call_count);

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
    CHECK(wp == rp);
}

TEST(shipper, packet_not_composed) {
    address::SocketAddr address;
    MockComposer composer(arena);
    FifoQueue queue;

    Shipper shipper(composer, queue, &address);

    PacketPtr wp = new_packet();

    CHECK((wp->flags() & Packet::FlagComposed) == 0);
    LONGS_EQUAL(0, composer.compose_call_count);

    LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    LONGS_EQUAL(1, composer.compose_call_count);
    CHECK(wp->flags() & Packet::FlagComposed);

    packet::PacketPtr rp;
    LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
    CHECK(wp == rp);
}

TEST(shipper, forward_error) {
    const status::StatusCode status_codes[] = {
        status::StatusOK,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_codes); ++st_n) {
        address::SocketAddr address;
        MockComposer composer(arena);
        MockWriter writer(status_codes[st_n]);

        Shipper shipper(composer, writer, &address);

        PacketPtr pp = new_packet();
        LONGS_EQUAL(status_codes[st_n], shipper.write(pp));
    }
}

} // namespace packet
} // namespace roc
