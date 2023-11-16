/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/pcm_format.h"
#include "roc_core/heap_arena.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace rtp {

core::HeapArena arena;

TEST_GROUP(format_map) {};

TEST(format_map, find_by_pt) {
    FormatMap fmt_map(arena);

    {
        const Format* fmt = fmt_map.find_by_pt(99);
        CHECK(!fmt);
    }

    {
        const Format* fmt = fmt_map.find_by_pt(PayloadType_L16_Mono);
        CHECK(fmt);

        LONGS_EQUAL(PayloadType_L16_Mono, fmt->payload_type);

        CHECK(fmt->pcm_format
              == audio::PcmFormat(audio::PcmCode_SInt16, audio::PcmEndian_Big));

        CHECK(fmt->sample_spec.is_valid());
        CHECK(fmt->sample_spec
              == audio::SampleSpec(44100, audio::ChanLayout_Surround,
                                   audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Mono));

        CHECK(fmt->packet_flags & packet::Packet::FlagAudio);

        CHECK(fmt->new_encoder);
        CHECK(fmt->new_decoder);
    }

    {
        const Format* fmt = fmt_map.find_by_pt(PayloadType_L16_Stereo);
        CHECK(fmt);

        LONGS_EQUAL(PayloadType_L16_Stereo, fmt->payload_type);

        CHECK(fmt->pcm_format
              == audio::PcmFormat(audio::PcmCode_SInt16, audio::PcmEndian_Big));

        CHECK(fmt->sample_spec.is_valid());
        CHECK(fmt->sample_spec
              == audio::SampleSpec(44100, audio::ChanLayout_Surround,
                                   audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(fmt->packet_flags & packet::Packet::FlagAudio);

        CHECK(fmt->new_encoder);
        CHECK(fmt->new_decoder);
    }
}

TEST(format_map, find_by_spec) {
    FormatMap fmt_map(arena);

    {
        const Format* fmt = fmt_map.find_by_spec(
            audio::SampleSpec(48000, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Mono));

        CHECK(!fmt);
    }

    {
        const Format* fmt = fmt_map.find_by_spec(
            audio::SampleSpec(44100, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Mono));

        CHECK(fmt);

        LONGS_EQUAL(PayloadType_L16_Mono, fmt->payload_type);
    }

    {
        const Format* fmt = fmt_map.find_by_spec(
            audio::SampleSpec(44100, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Stereo));

        CHECK(fmt);

        LONGS_EQUAL(PayloadType_L16_Stereo, fmt->payload_type);
    }
}

TEST(format_map, add_format) {
    FormatMap fmt_map(arena);

    {
        Format fmt;
        fmt.payload_type = (PayloadType)100;
        fmt.packet_flags = packet::Packet::FlagAudio;
        fmt.pcm_format =
            audio::PcmFormat(audio::PcmCode_Float32, audio::PcmEndian_Native);
        fmt.sample_spec =
            audio::SampleSpec(48000, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Stereo);
        fmt.new_encoder = &audio::PcmEncoder::construct;
        fmt.new_decoder = &audio::PcmDecoder::construct;

        CHECK(fmt_map.add_format(fmt));
    }

    {
        const Format* fmt = fmt_map.find_by_pt(100);
        CHECK(fmt);

        LONGS_EQUAL(100, fmt->payload_type);

        CHECK(fmt->pcm_format
              == audio::PcmFormat(audio::PcmCode_Float32, audio::PcmEndian_Native));

        CHECK(fmt->sample_spec
              == audio::SampleSpec(48000, audio::ChanLayout_Surround,
                                   audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(fmt->packet_flags == packet::Packet::FlagAudio);

        CHECK(fmt->new_encoder);
        CHECK(fmt->new_decoder);
    }

    {
        const Format* fmt = fmt_map.find_by_spec(
            audio::SampleSpec(48000, audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                              audio::ChanMask_Surround_Stereo));
        CHECK(fmt);

        LONGS_EQUAL(100, fmt->payload_type);

        CHECK(fmt->pcm_format
              == audio::PcmFormat(audio::PcmCode_Float32, audio::PcmEndian_Native));

        CHECK(fmt->sample_spec
              == audio::SampleSpec(48000, audio::ChanLayout_Surround,
                                   audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(fmt->packet_flags == packet::Packet::FlagAudio);

        CHECK(fmt->new_encoder);
        CHECK(fmt->new_decoder);
    }
}

} // namespace rtp
} // namespace roc
