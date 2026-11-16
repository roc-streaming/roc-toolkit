// /*
//  * Copyright (c) 2015 Roc Streaming authors
//  *
//  * This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this
//  * file, You can obtain one at http://mozilla.org/MPL/2.0/.
//  */

// #include <CppUTest/TestHarness.h>

// #include "roc_core/heap_arena.h"
// #include "roc_core/macro_helpers.h"
// #include "roc_packet/delayed_reader.h"
// #include "roc_packet/packet_factory.h"
// #include "roc_packet/queue.h"
// #include "roc_status/status_code.h"

// namespace roc {
// namespace packet {

// namespace {

// enum { SampleRate = 1000, NumSamples = 100, NumPackets = 30, MaxBufSize = 100 };

// const core::nanoseconds_t NsPerSample = core::Second / SampleRate;

// const audio::SampleSpec sample_spec(SampleRate,
//                                     audio::Sample_RawFormat,
//                                     audio::ChanLayout_Surround,
//                                     audio::ChanOrder_Smpte,
//                                     audio::ChanMask_Surround_Stereo);

// core::HeapArena arena;
// PacketFactory packet_factory(arena, MaxBufSize);

// PacketPtr new_packet(seqnum_t sn) {
//     PacketPtr packet = packet_factory.new_packet();
//     CHECK(packet);

//     packet->add_flags(Packet::FlagRTP);
//     packet->rtp()->seqnum = sn;
//     packet->rtp()->stream_timestamp = stream_timestamp_t(sn * NumSamples);

//     return packet;
// }

// class StatusReader : public IReader {
// public:
//     explicit StatusReader(status::StatusCode code)
//         : code_(code) {
//     }

//     virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr&) {
//         return code_;
//     }

// private:
//     status::StatusCode code_;
// };

// } // namespace

// TEST_GROUP(delayed_reader) {};

// TEST(delayed_reader, failed_to_read_packet) {
//     const status::StatusCode codes[] = {
//         status::StatusUnknown,
//         status::StatusNoData,
//     };

//     for (size_t n = 0; n < ROC_ARRAY_SIZE(codes); ++n) {
//         StatusReader reader(codes[n]);
//         DelayedReader dr(reader, sample_spec);
//         CHECK(dr.is_valid());

//         PacketPtr pp;
//         LONGS_EQUAL(codes[n], dr.read(pp));
//         CHECK(!pp);
//     }
// }

// TEST(delayed_reader, no_delay) {
//     Queue queue;
//     DelayedReader dr(queue, sample_spec);
//     CHECK(dr.is_valid());

//     PacketPtr pp;
//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr wp = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(wp));

//         PacketPtr rp;
//         LONGS_EQUAL(status::StatusOK, dr.read(rp));
//         CHECK(wp == rp);
//     }
// }

// TEST(delayed_reader, delay) {
//     Queue queue;
//     DelayedReader dr(queue, sample_spec);
//     CHECK(dr.is_valid());

//     PacketPtr packets[NumPackets];

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr p;
//         LONGS_EQUAL(status::StatusNoData, dr.read(p));
//         CHECK(!p);

//         packets[n] = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(packets[n]));
//     }

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr p;
//         LONGS_EQUAL(status::StatusOK, dr.read(p));
//         CHECK(p == packets[n]);
//     }

//     PacketPtr pp;
//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr wp = new_packet(NumPackets + n);
//         LONGS_EQUAL(status::StatusOK, queue.write(wp));

//         PacketPtr rp;
//         LONGS_EQUAL(status::StatusOK, dr.read(rp));
//         CHECK(wp == rp);
//     }

//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);
// }

// TEST(delayed_reader, instant) {
//     Queue queue;
//     DelayedReader dr(queue, sample_spec);
//     CHECK(dr.is_valid());

//     PacketPtr packets[NumPackets];

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         packets[n] = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(packets[n]));
//     }

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr p;
//         LONGS_EQUAL(status::StatusOK, dr.read(p));
//         CHECK(p == packets[n]);
//     }

//     PacketPtr pp;
//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);
// }

// TEST(delayed_reader, trim) {
//     Queue queue;
//     DelayedReader dr(queue, sample_spec);
//     CHECK(dr.is_valid());

//     PacketPtr packets[NumPackets * 2];

//     for (seqnum_t n = 0; n < NumPackets * 2; n++) {
//         packets[n] = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(packets[n]));
//     }

//     for (seqnum_t n = NumPackets; n < NumPackets * 2; n++) {
//         PacketPtr p;
//         LONGS_EQUAL(status::StatusOK, dr.read(p));
//         CHECK(p == packets[n]);
//     }

//     PacketPtr pp;
//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);
// }

// TEST(delayed_reader, late_duplicates) {
//     Queue queue;
//     DelayedReader dr(queue, sample_spec);
//     CHECK(dr.is_valid());

//     PacketPtr packets[NumPackets];

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         packets[n] = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(packets[n]));
//     }

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr p;
//         LONGS_EQUAL(status::StatusOK, dr.read(p));
//         CHECK(p == packets[n]);
//     }

//     for (seqnum_t n = 0; n < NumPackets; n++) {
//         PacketPtr wp = new_packet(n);
//         LONGS_EQUAL(status::StatusOK, queue.write(wp));

//         PacketPtr rp;
//         LONGS_EQUAL(status::StatusOK, dr.read(rp));
//         CHECK(wp == rp);
//     }

//     PacketPtr pp;
//     LONGS_EQUAL(status::StatusNoData, dr.read(pp));
//     CHECK(!pp);
// }

// } // namespace packet
// } // namespace roc
