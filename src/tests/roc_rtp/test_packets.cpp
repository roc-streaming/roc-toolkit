/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/stddefs.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

#include "test_packets/rtp_l16_1ch_10s_12ext.h"
#include "test_packets/rtp_l16_1ch_10s_4pad_2csrc_12ext_marker.h"
#include "test_packets/rtp_l16_2ch_320s.h"

namespace roc {
namespace rtp {

namespace {

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, PacketInfo::MaxData, 1);
packet::PacketPool packet_pool(allocator, 1);

} // namespace

TEST_GROUP(packets) {
    core::Slice<uint8_t> new_buffer(const uint8_t* data, size_t datasz) {
        core::Slice<uint8_t> buf = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        if (data) {
            buf.resize(datasz);
            memcpy(buf.data(), data, datasz);
        }
        return buf;
    }

    packet::PacketPtr new_packet() {
        return new(packet_pool) packet::Packet(packet_pool);
    }

    void check_packet_info(const PacketInfo& pi) {
        UNSIGNED_LONGS_EQUAL(V2, pi.version);

        UNSIGNED_LONGS_EQUAL(pi.packet_size,
                             pi.header_size + pi.extension_size + pi.payload_size
                                 + pi.padding_size);
    }

    void check_format(const Format& format,
                      packet::Packet& packet,
                      const PacketInfo& pi,
                      bool check_size) {
        UNSIGNED_LONGS_EQUAL(packet::Packet::FlagAudio, format.flags);
        UNSIGNED_LONGS_EQUAL(pi.pt, format.payload_type);
        UNSIGNED_LONGS_EQUAL(pi.samplerate, format.sample_rate);
        UNSIGNED_LONGS_EQUAL(pi.num_channels, packet::num_channels(format.channel_mask));

        CHECK(packet.rtp());
        UNSIGNED_LONGS_EQUAL(pi.num_samples, format.duration(*packet.rtp()));

        if (check_size) {
            UNSIGNED_LONGS_EQUAL(pi.packet_size, format.size(pi.num_samples));
        }
    }

    void check_headers(const packet::Packet& packet, const PacketInfo& pi) {
        UNSIGNED_LONGS_EQUAL(packet::Packet::FlagRTP | packet::Packet::FlagAudio,
                             packet.flags());

        CHECK(packet.data());
        CHECK(packet.rtp());
        CHECK(packet.rtp()->header);
        CHECK(packet.rtp()->payload);

        UNSIGNED_LONGS_EQUAL(pi.packet_size, packet.data().size());
        UNSIGNED_LONGS_EQUAL(pi.header_size + pi.extension_size,
                             packet.rtp()->header.size());
        UNSIGNED_LONGS_EQUAL(pi.payload_size, packet.rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(pi.ssrc, packet.rtp()->source);
        UNSIGNED_LONGS_EQUAL(pi.seqnum, packet.rtp()->seqnum);
        UNSIGNED_LONGS_EQUAL(pi.ts, packet.rtp()->timestamp);
        UNSIGNED_LONGS_EQUAL(pi.marker, packet.rtp()->marker);
        UNSIGNED_LONGS_EQUAL(pi.pt, packet.rtp()->payload_type);
        UNSIGNED_LONGS_EQUAL(pi.num_samples, packet.rtp()->duration);
    }

    void set_headers(packet::Packet& packet, const PacketInfo& pi) {
        CHECK(packet.rtp());

        packet.rtp()->source = pi.ssrc;
        packet.rtp()->seqnum = pi.seqnum;
        packet.rtp()->timestamp = pi.ts;
        packet.rtp()->marker = pi.marker;
        packet.rtp()->payload_type = pi.pt;
    }

    void check_data(packet::Packet& packet, const PacketInfo& pi) {
        CHECK(packet.data());

        CHECK(packet.rtp());
        CHECK(packet.rtp()->header);
        CHECK(packet.rtp()->payload);

        UNSIGNED_LONGS_EQUAL(pi.packet_size, packet.data().size());

        UNSIGNED_LONGS_EQUAL(packet.data().size(),
                             packet.rtp()->header.size() + packet.rtp()->payload.size());

        CHECK(memcmp(packet.data().data(), pi.raw_data, pi.packet_size) == 0);
    }

    void decode_samples(audio::IDecoder& decoder,
                        const packet::Packet& packet,
                        const PacketInfo& pi) {
        audio::sample_t samples[PacketInfo::MaxSamples * PacketInfo::MaxCh] = {};

        UNSIGNED_LONGS_EQUAL(
            pi.num_samples,
            decoder.read_samples(packet, 0, samples, pi.num_samples,
                                 packet::channel_mask_t(1 << pi.num_channels) - 1));

        size_t i = 0;

        for (size_t ns = 0; ns < pi.num_samples; ns++) {
            for (size_t nch = 0; nch < pi.num_channels; nch++) {
                LONGS_EQUAL(pi.samples[nch][ns],
                            long(samples[i] * (1 << (pi.samplebits - 1))));
                i++;
            }
        }
    }

    void encode_samples(audio::IEncoder& encoder,
                        packet::Packet& packet,
                        const PacketInfo& pi) {
        audio::sample_t samples[PacketInfo::MaxSamples * PacketInfo::MaxCh] = {};

        size_t i = 0;

        for (size_t ns = 0; ns < pi.num_samples; ns++) {
            for (size_t nch = 0; nch < pi.num_channels; nch++) {
                samples[i] = audio::sample_t(pi.samples[nch][ns])
                    / (1 << (pi.samplebits - 1));
                i++;
            }
        }

        UNSIGNED_LONGS_EQUAL(
            pi.num_samples,
            encoder.write_samples(packet, 0, samples, pi.num_samples,
                                  packet::channel_mask_t(1 << pi.num_channels) - 1));
    }

    void check_parse_decode(const PacketInfo& pi) {
        FormatMap format_map;

        core::Slice<uint8_t> buffer = new_buffer(pi.raw_data, pi.packet_size);
        CHECK(buffer);

        packet::PacketPtr packet = new_packet();
        CHECK(packet);

        packet->set_data(buffer);

        Parser parser(format_map, NULL);
        CHECK(parser.parse(*packet, packet->data()));

        const Format* format = format_map.format(packet->rtp()->payload_type);
        CHECK(format);

        core::UniquePtr<audio::IDecoder> decoder(format->new_decoder(allocator),
                                                 allocator);
        CHECK(decoder);

        check_format(*format, *packet, pi, false);
        check_headers(*packet, pi);

        decode_samples(*decoder, *packet, pi);
    }

    void check_compose_encode(const PacketInfo& pi) {
        FormatMap format_map;

        core::Slice<uint8_t> buffer = new_buffer(NULL, 0);
        CHECK(buffer);

        packet::PacketPtr packet = new_packet();
        CHECK(packet);

        const Format* format = format_map.format(pi.pt);
        CHECK(format);

        core::UniquePtr<audio::IEncoder> encoder(format->new_encoder(allocator),
                                                 allocator);
        CHECK(encoder);

        Composer composer(NULL);
        CHECK(composer.prepare(*packet, buffer, encoder->payload_size(pi.num_samples)));
        packet->set_data(buffer);

        encode_samples(*encoder, *packet, pi);
        set_headers(*packet, pi);

        CHECK(composer.compose(*packet));

        check_format(*format, *packet, pi, true);
        check_data(*packet, pi);
    }

    void check(const PacketInfo& pi, bool compose) {
        check_packet_info(pi);
        check_parse_decode(pi);
        if (compose) {
            check_compose_encode(pi);
        }
    }
};

TEST(packets, l16_2ch_320s) {
    check(rtp_l16_2ch_320s, true);
}

TEST(packets, l16_1ch_10s_12ext) {
    check(rtp_l16_1ch_10s_12ext, false);
}

TEST(packets, l16_1ch_10s_4pad_2csrc_12ext_marker) {
    check(rtp_l16_1ch_10s_4pad_2csrc_12ext_marker, false);
}

} // namespace rtp
} // namespace roc
