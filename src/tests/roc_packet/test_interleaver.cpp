/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace packet {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_packet(seqnum_t sn) {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(Packet::FlagRTP);
    packet->rtp()->seqnum = sn;

    return packet;
}

class MockWriter : public IWriter, public core::NonCopyable<> {
public:
    explicit MockWriter(IWriter& writer)
        : writer_(writer)
        , call_count_(0)
        , code_enabled_(false)
        , code_(status::NoStatus) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& pp) {
        ++call_count_;

        if (code_enabled_) {
            return code_;
        }

        return writer_.write(pp);
    }

    unsigned call_count() const {
        return call_count_;
    }

    void enable_status_code(status::StatusCode code) {
        code_enabled_ = true;
        code_ = code;
    }

    void disable_status_code() {
        code_enabled_ = false;
        code_ = status::NoStatus;
    }

private:
    IWriter& writer_;

    unsigned call_count_;
    bool code_enabled_;
    status::StatusCode code_;
};

} // namespace

TEST_GROUP(interleaver) {};

// Fill Interleaver with multiple of its internal memory size.
TEST(interleaver, read_write) {
    FifoQueue queue;
    Interleaver intrlvr(queue, arena, 10);
    LONGS_EQUAL(status::StatusOK, intrlvr.init_status());

    const size_t num_packets = intrlvr.block_size() * 5;

    // Packets to push to Interleaver.
    core::Array<PacketPtr> packets(arena);
    CHECK(packets.resize(num_packets));

    // Checks for received packets.
    core::Array<bool> packets_ctr(arena);
    CHECK(packets_ctr.resize(num_packets));

    for (size_t i = 0; i < num_packets; i++) {
        packets[i] = new_packet(seqnum_t(i));
        packets_ctr[i] = false;
    }

    // No packets in interleaver on start.
    LONGS_EQUAL(0, queue.size());

    // Push every packet to interleaver.
    for (size_t i = 0; i < num_packets; i++) {
        LONGS_EQUAL(status::StatusOK, intrlvr.write(packets[i]));
    }

    // Interleaver must put all packets to its writer because we put precisely
    // integer number of its block_size.
    LONGS_EQUAL(num_packets, queue.size());

    // Check that packets have different seqnums.
    for (size_t i = 0; i < num_packets; i++) {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp, ModeFetch));
        CHECK(pp);
        CHECK(pp->rtp()->seqnum < num_packets);
        CHECK(!packets_ctr[pp->rtp()->seqnum]);
        packets_ctr[pp->rtp()->seqnum] = true;
    }

    // Nothing left in queue.
    LONGS_EQUAL(0, queue.size());
    LONGS_EQUAL(status::StatusOK, intrlvr.flush());

    // Nothing left in interleaver.
    LONGS_EQUAL(0, queue.size());

    // Did we receive all packets that we've sent.
    for (size_t i = 0; i < num_packets; i++) {
        CHECK(packets_ctr[i]);
    }
}

TEST(interleaver, flush) {
    FifoQueue queue;
    Interleaver intrlvr(queue, arena, 10);
    LONGS_EQUAL(status::StatusOK, intrlvr.init_status());

    const size_t num_packets = intrlvr.block_size() * 5;

    for (size_t n = 0; n < num_packets; n++) {
        PacketPtr wp = new_packet(seqnum_t(n));

        LONGS_EQUAL(status::StatusOK, intrlvr.write(wp));
        LONGS_EQUAL(status::StatusOK, intrlvr.flush());
        LONGS_EQUAL(1, queue.size());

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
        LONGS_EQUAL(0, queue.size());
    }
}

TEST(interleaver, write_error) {
    const status::StatusCode status_codes[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_codes); ++st_n) {
        FifoQueue queue;
        MockWriter writer(queue);
        Interleaver intrlvr(writer, arena, 1);
        LONGS_EQUAL(status::StatusOK, intrlvr.init_status());

        writer.enable_status_code(status_codes[st_n]);

        PacketPtr wp = new_packet(seqnum_t(1));

        UNSIGNED_LONGS_EQUAL(status_codes[st_n], intrlvr.write(wp));
        UNSIGNED_LONGS_EQUAL(1, writer.call_count());
        UNSIGNED_LONGS_EQUAL(0, queue.size());

        writer.disable_status_code();
        LONGS_EQUAL(status::StatusOK, intrlvr.write(wp));
        UNSIGNED_LONGS_EQUAL(2, writer.call_count());
        UNSIGNED_LONGS_EQUAL(1, queue.size());

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
    }
}

TEST(interleaver, flush_error) {
    const size_t block_size = 10;

    FifoQueue queue;
    MockWriter writer(queue);
    Interleaver intrlvr(writer, arena, block_size);
    LONGS_EQUAL(status::StatusOK, intrlvr.init_status());

    writer.enable_status_code(status::StatusAbort);
    LONGS_EQUAL(status::StatusOK, intrlvr.flush());

    size_t seqnum = 0;

    while (true) {
        PacketPtr pp = new_packet(seqnum_t(seqnum));
        ++seqnum;

        const status::StatusCode code = intrlvr.write(pp);
        if (code != status::StatusOK) {
            LONGS_EQUAL(status::StatusAbort, code);
            break;
        }
    }

    UNSIGNED_LONGS_EQUAL(1, writer.call_count());
    UNSIGNED_LONGS_EQUAL(0, queue.size());

    LONGS_EQUAL(status::StatusAbort, intrlvr.flush());

    UNSIGNED_LONGS_EQUAL(2, writer.call_count());
    UNSIGNED_LONGS_EQUAL(0, queue.size());

    writer.disable_status_code();
    LONGS_EQUAL(status::StatusOK, intrlvr.flush());
    UNSIGNED_LONGS_EQUAL(seqnum, queue.size());

    for (size_t n = 0; n < seqnum; ++n) {
        PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, queue.read(pp, ModeFetch));
        UNSIGNED_LONGS_EQUAL(seqnum_t(n), pp->rtp()->seqnum);
    }
}

} // namespace packet
} // namespace roc
