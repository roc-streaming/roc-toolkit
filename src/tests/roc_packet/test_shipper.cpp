/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_packet/shipper.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace packet {

namespace {

class StatusWriter : public IWriter, public core::NonCopyable<> {
public:
    explicit StatusWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr&) {
        return code_;
    }

private:
    status::StatusCode code_;
};

struct TestComposer : public IComposer, public core::NonCopyable<> {
    TestComposer()
        : compose_call_count(0) {
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

PacketPtr new_packet(PacketFactory& packet_factory,
                     core::BufferFactory<uint8_t>& buffer_factory) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP | Packet::FlagPrepared);
    packet->rtp()->payload_type = rtp::PayloadType_L16_Stereo;

    core::Slice<uint8_t> buffer = buffer_factory.new_buffer();
    CHECK(buffer);
    packet->rtp()->payload = buffer;

    return packet;
}

enum { PacketSz = 128 };

core::HeapArena arena;
PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);

} // namespace

TEST_GROUP(shipper) {};

TEST(shipper, verify_proxy_write_status_code_as_is) {
    const status::StatusCode codes[] = {
        status::StatusOK,
        status::StatusUnknown,
        status::StatusNoData,
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
        address::SocketAddr addr;
        TestComposer composer;
        StatusWriter writer(codes[n]);

        Shipper shipper(addr, composer, writer);

        PacketPtr pp = new_packet(packet_factory, buffer_factory);
        UNSIGNED_LONGS_EQUAL(codes[n], shipper.write(pp));
    }
}

TEST(shipper, verify_flags_if_port_and_host_is_not_set) {
    address::SocketAddr addr;
    TestComposer composer;
    Queue queue;

    Shipper shipper(addr, composer, queue);

    PacketPtr wp = new_packet(packet_factory, buffer_factory);

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    UNSIGNED_LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK(wp == rp);
}

TEST(shipper, verify_flags_if_port_and_host_is_set) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port_auto("127.0.0.1", 0));

    TestComposer composer;
    Queue queue;

    Shipper shipper(addr, composer, queue);

    PacketPtr wp = new_packet(packet_factory, buffer_factory);

    CHECK((wp->flags() & Packet::FlagUDP) == 0);
    CHECK(!wp->udp());

    UNSIGNED_LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK(wp->flags() & Packet::FlagUDP);
    CHECK(addr == wp->udp()->dst_addr);

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK(wp == rp);
}

TEST(shipper, verify_flags_if_packet_is_composed) {
    address::SocketAddr addr;
    TestComposer composer;
    Queue queue;

    Shipper shipper(addr, composer, queue);

    PacketPtr wp = new_packet(packet_factory, buffer_factory);
    wp->add_flags(Packet::FlagComposed);

    CHECK(wp->flags() & Packet::FlagComposed);
    UNSIGNED_LONGS_EQUAL(0, composer.compose_call_count);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    CHECK(wp->flags() & Packet::FlagComposed);
    UNSIGNED_LONGS_EQUAL(0, composer.compose_call_count);

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK(wp == rp);
}

TEST(shipper, verify_flags_if_packet_is_not_composed) {
    address::SocketAddr addr;
    TestComposer composer;
    Queue queue;

    Shipper shipper(addr, composer, queue);

    PacketPtr wp = new_packet(packet_factory, buffer_factory);

    CHECK((wp->flags() & Packet::FlagComposed) == 0);
    UNSIGNED_LONGS_EQUAL(0, composer.compose_call_count);

    UNSIGNED_LONGS_EQUAL(status::StatusOK, shipper.write(wp));

    UNSIGNED_LONGS_EQUAL(1, composer.compose_call_count);
    CHECK(wp->flags() & Packet::FlagComposed);

    packet::PacketPtr rp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(rp));
    CHECK(wp == rp);
}

} // namespace packet
} // namespace roc
