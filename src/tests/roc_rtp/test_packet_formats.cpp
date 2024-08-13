/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_packets/rtp_l16_1ch_10s_12ext.h"
#include "test_packets/rtp_l16_1ch_10s_4pad_2csrc_12ext_marker.h"
#include "test_packets/rtp_l16_2ch_300s_80pad.h"
#include "test_packets/rtp_l16_2ch_320s.h"

#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace rtp {

namespace {

enum { MaxBufSize = test::PacketInfo::MaxData };

enum { CanParse = (1 << 0), CanCompose = (1 << 1) };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);

core::Slice<uint8_t> new_buffer(const uint8_t* data, size_t datasz) {
    core::Slice<uint8_t> buf = packet_factory.new_packet_buffer();
    if (data) {
        buf.reslice(0, datasz);
        memcpy(buf.data(), data, datasz);
    }
    return buf;
}

void check_packet_info(const test::PacketInfo& pi) {
    UNSIGNED_LONGS_EQUAL(V2, pi.version);

    UNSIGNED_LONGS_EQUAL(pi.packet_size,
                         pi.header_size + pi.extension_size + pi.payload_size
                             + pi.padding_size);
}

void check_format_info(const Encoding& enc, const test::PacketInfo& pi) {
    UNSIGNED_LONGS_EQUAL(packet::Packet::FlagAudio, enc.packet_flags);
    UNSIGNED_LONGS_EQUAL(pi.pt, enc.payload_type);
    UNSIGNED_LONGS_EQUAL(pi.samplerate, enc.sample_spec.sample_rate());
    UNSIGNED_LONGS_EQUAL(pi.num_channels, enc.sample_spec.num_channels());
}

void check_packet_fields(const packet::Packet& packet, const test::PacketInfo& pi) {
    UNSIGNED_LONGS_EQUAL(packet::Packet::FlagRTP | packet::Packet::FlagAudio,
                         packet.flags());

    CHECK(packet.buffer());
    CHECK(packet.rtp());
    CHECK(packet.rtp()->header);
    CHECK(packet.rtp()->payload);
    if (pi.padding) {
        CHECK(packet.rtp()->padding);
    }

    UNSIGNED_LONGS_EQUAL(pi.packet_size, packet.buffer().size());
    UNSIGNED_LONGS_EQUAL(pi.header_size + pi.extension_size, packet.rtp()->header.size());
    UNSIGNED_LONGS_EQUAL(pi.payload_size, packet.rtp()->payload.size());
    UNSIGNED_LONGS_EQUAL(pi.padding_size, packet.rtp()->padding.size());

    UNSIGNED_LONGS_EQUAL(pi.ssrc, packet.rtp()->source_id);
    UNSIGNED_LONGS_EQUAL(pi.seqnum, packet.rtp()->seqnum);
    UNSIGNED_LONGS_EQUAL(pi.ts, packet.rtp()->stream_timestamp);
    UNSIGNED_LONGS_EQUAL(pi.marker, packet.rtp()->marker);
    UNSIGNED_LONGS_EQUAL(pi.pt, packet.rtp()->payload_type);
    UNSIGNED_LONGS_EQUAL(pi.padding, (packet.rtp()->padding.size() != 0));
}

void set_packet_fields(packet::Packet& packet, const test::PacketInfo& pi) {
    CHECK(packet.rtp());

    packet.rtp()->source_id = pi.ssrc;
    packet.rtp()->seqnum = pi.seqnum;
    packet.rtp()->stream_timestamp = pi.ts;
    packet.rtp()->marker = pi.marker;
    packet.rtp()->payload_type = pi.pt;
}

void check_packet_data(packet::Packet& packet, const test::PacketInfo& pi) {
    CHECK(packet.buffer());

    CHECK(packet.rtp());
    CHECK(packet.rtp()->header);
    CHECK(packet.rtp()->payload);

    UNSIGNED_LONGS_EQUAL(pi.packet_size, packet.buffer().size());

    UNSIGNED_LONGS_EQUAL(packet.buffer().size(),
                         packet.rtp()->header.size() + packet.rtp()->payload.size()
                             + packet.rtp()->padding.size());

    CHECK(memcmp(packet.buffer().data(), pi.raw_data, pi.packet_size) == 0);
}

void decode_samples(audio::IFrameDecoder& decoder,
                    const packet::Packet& packet,
                    const test::PacketInfo& pi) {
    audio::sample_t samples[test::PacketInfo::MaxSamples * test::PacketInfo::MaxCh] = {};

    decoder.begin_frame(packet.rtp()->stream_timestamp, packet.rtp()->payload.data(),
                        packet.rtp()->payload.size());

    UNSIGNED_LONGS_EQUAL(pi.num_samples, decoder.read_samples(samples, pi.num_samples));

    decoder.end_frame();

    size_t i = 0;

    for (size_t ns = 0; ns < pi.num_samples; ns++) {
        for (size_t nch = 0; nch < pi.num_channels; nch++) {
            LONGS_EQUAL(pi.samples[nch][ns],
                        long(samples[i] * (1 << (pi.samplebits - 1))));
            i++;
        }
    }
}

void encode_samples(audio::IFrameEncoder& encoder,
                    packet::Packet& packet,
                    const test::PacketInfo& pi) {
    audio::sample_t samples[test::PacketInfo::MaxSamples * test::PacketInfo::MaxCh] = {};

    size_t i = 0;

    for (size_t ns = 0; ns < pi.num_samples; ns++) {
        for (size_t nch = 0; nch < pi.num_channels; nch++) {
            samples[i] =
                audio::sample_t(pi.samples[nch][ns]) / (1 << (pi.samplebits - 1));
            i++;
        }
    }

    UNSIGNED_LONGS_EQUAL(pi.payload_size, encoder.encoded_byte_count(pi.num_samples));

    encoder.begin_frame(packet.rtp()->payload.data(), packet.rtp()->payload.size());

    UNSIGNED_LONGS_EQUAL(pi.num_samples, encoder.write_samples(samples, pi.num_samples));

    encoder.end_frame();
}

void check_parse_decode(const test::PacketInfo& pi) {
    EncodingMap encoding_map(arena);

    core::Slice<uint8_t> buffer = new_buffer(pi.raw_data, pi.packet_size);
    CHECK(buffer);

    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->set_buffer(buffer);

    Parser parser(NULL, encoding_map, arena);
    CHECK(parser.parse(*packet, packet->buffer()));

    const Encoding* encoding = encoding_map.find_by_pt(packet->rtp()->payload_type);
    CHECK(encoding);

    core::ScopedPtr<audio::IFrameDecoder> decoder(
        encoding->new_decoder(encoding->sample_spec, arena));
    CHECK(decoder);

    check_format_info(*encoding, pi);
    check_packet_fields(*packet, pi);
    check_packet_data(*packet, pi);

    decode_samples(*decoder, *packet, pi);
}

void check_compose_encode(const test::PacketInfo& pi) {
    EncodingMap encoding_map(arena);

    core::Slice<uint8_t> buffer = new_buffer(NULL, 0);
    CHECK(buffer);

    packet::PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);

    packet->add_flags(packet::Packet::FlagAudio);

    const Encoding* encoding = encoding_map.find_by_pt(pi.pt);
    CHECK(encoding);

    core::ScopedPtr<audio::IFrameEncoder> encoder(
        encoding->new_encoder(encoding->sample_spec, arena));
    CHECK(encoder);

    Composer composer(NULL, arena);

    CHECK(composer.prepare(*packet, buffer, pi.payload_size + pi.padding_size));
    packet->set_buffer(buffer);

    encode_samples(*encoder, *packet, pi);
    set_packet_fields(*packet, pi);

    if (pi.padding_size != 0) {
        composer.pad(*packet, pi.padding_size);
    }

    CHECK(composer.compose(*packet));

    check_format_info(*encoding, pi);
    check_packet_fields(*packet, pi);
    check_packet_data(*packet, pi);
}

void check(const test::PacketInfo& pi, unsigned flags) {
    check_packet_info(pi);

    if (flags & CanParse) {
        check_parse_decode(pi);
    }

    if (flags & CanCompose) {
        check_compose_encode(pi);
    }
}

} // namespace

TEST_GROUP(packet_formats) {};

TEST(packet_formats, l16_2ch_320s) {
    check(test::rtp_l16_2ch_320s, CanParse | CanCompose);
}

TEST(packet_formats, l16_2ch_300s_80pad) {
    check(test::rtp_l16_2ch_300s_80pad, CanParse | CanCompose);
}

TEST(packet_formats, l16_1ch_10s_12ext) {
    check(test::rtp_l16_1ch_10s_12ext, CanParse);
}

TEST(packet_formats, l16_1ch_10s_4pad_2csrc_12ext_marker) {
    check(test::rtp_l16_1ch_10s_4pad_2csrc_12ext_marker, CanParse);
}

} // namespace rtp
} // namespace roc
