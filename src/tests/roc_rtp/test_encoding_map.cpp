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
#include "roc_audio/pcm_subformat.h"
#include "roc_core/heap_arena.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace rtp {

core::HeapArena arena;

TEST_GROUP(encoding_map) {};

TEST(encoding_map, find_by_pt) {
    EncodingMap enc_map(arena);

    {
        const Encoding* enc = enc_map.find_by_pt(99);
        CHECK(!enc);
    }

    {
        const Encoding* enc = enc_map.find_by_pt(PayloadType_L16_Mono);
        CHECK(enc);

        LONGS_EQUAL(PayloadType_L16_Mono, enc->payload_type);

        CHECK(enc->sample_spec.is_complete());
        CHECK(enc->sample_spec
              == audio::SampleSpec(44100, audio::PcmSubformat_SInt16_Be,
                                   audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Mono));

        CHECK(enc->packet_flags & packet::Packet::FlagAudio);

        CHECK(enc->new_encoder);
        CHECK(enc->new_decoder);
    }

    {
        const Encoding* enc = enc_map.find_by_pt(PayloadType_L16_Stereo);
        CHECK(enc);

        LONGS_EQUAL(PayloadType_L16_Stereo, enc->payload_type);

        CHECK(enc->sample_spec.is_complete());
        CHECK(enc->sample_spec
              == audio::SampleSpec(44100, audio::PcmSubformat_SInt16_Be,
                                   audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(enc->packet_flags & packet::Packet::FlagAudio);

        CHECK(enc->new_encoder);
        CHECK(enc->new_decoder);
    }
}

TEST(encoding_map, find_by_spec) {
    EncodingMap enc_map(arena);

    {
        const Encoding* enc = enc_map.find_by_spec(audio::SampleSpec(
            48000, audio::PcmSubformat_SInt16_Be, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Mono));

        CHECK(!enc);
    }

    {
        const Encoding* enc = enc_map.find_by_spec(audio::SampleSpec(
            44100, audio::PcmSubformat_SInt16_Be, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Mono));

        CHECK(enc);

        LONGS_EQUAL(PayloadType_L16_Mono, enc->payload_type);
    }

    {
        const Encoding* enc = enc_map.find_by_spec(audio::SampleSpec(
            44100, audio::PcmSubformat_SInt16_Be, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo));

        CHECK(enc);

        LONGS_EQUAL(PayloadType_L16_Stereo, enc->payload_type);
    }
}

TEST(encoding_map, add_encoding) {
    EncodingMap enc_map(arena);

    {
        Encoding enc;
        enc.payload_type = (PayloadType)100;
        enc.packet_flags = packet::Packet::FlagAudio;
        enc.sample_spec = audio::SampleSpec(
            48000, audio::PcmSubformat_SInt32, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo);
        enc.new_encoder = &audio::PcmEncoder::construct;
        enc.new_decoder = &audio::PcmDecoder::construct;

        LONGS_EQUAL(status::StatusOK, enc_map.register_encoding(enc));
    }

    {
        const Encoding* enc = enc_map.find_by_pt(100);
        CHECK(enc);

        LONGS_EQUAL(100, enc->payload_type);

        CHECK(enc->sample_spec
              == audio::SampleSpec(48000, audio::PcmSubformat_SInt32,
                                   audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(enc->packet_flags == packet::Packet::FlagAudio);

        CHECK(enc->new_encoder);
        CHECK(enc->new_decoder);
    }

    {
        const Encoding* enc = enc_map.find_by_spec(audio::SampleSpec(
            48000, audio::PcmSubformat_SInt32, audio::ChanLayout_Surround,
            audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo));
        CHECK(enc);

        LONGS_EQUAL(100, enc->payload_type);

        CHECK(enc->sample_spec
              == audio::SampleSpec(48000, audio::PcmSubformat_SInt32,
                                   audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                   audio::ChanMask_Surround_Stereo));

        CHECK(enc->packet_flags == packet::Packet::FlagAudio);

        CHECK(enc->new_encoder);
        CHECK(enc->new_decoder);
    }
}

} // namespace rtp
} // namespace roc
