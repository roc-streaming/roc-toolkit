/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtp/encoding.h"

namespace roc {
namespace rtp {

TEST_GROUP(encoding) {};

TEST(encoding, parse) {
    Encoding enc;
    CHECK(parse_encoding("101:s18/48000/surround4.1", enc));

    CHECK_EQUAL(101, enc.payload_type);

    CHECK(enc.sample_spec.is_valid());
    CHECK_EQUAL(audio::SampleFormat_Pcm, enc.sample_spec.sample_format());
    CHECK_EQUAL(audio::PcmFormat_SInt18, enc.sample_spec.pcm_format());
    CHECK_EQUAL(48000, enc.sample_spec.sample_rate());
    CHECK_EQUAL(5, enc.sample_spec.num_channels());

    CHECK_EQUAL(packet::Packet::FlagAudio, enc.packet_flags);
    CHECK(enc.new_encoder == NULL);
    CHECK(enc.new_decoder == NULL);
}

TEST(encoding, parse_errors) {
    Encoding enc;

    CHECK(!parse_encoding(":s16/44100/stereo", enc));
    CHECK(!parse_encoding("101,s16/44100/stereo", enc));
    CHECK(!parse_encoding("101:", enc));
    CHECK(!parse_encoding("101:s16/44100/bad", enc));
    CHECK(!parse_encoding(":", enc));
    CHECK(!parse_encoding("", enc));
    CHECK(!parse_encoding("::", enc));
    CHECK(!parse_encoding("101::s16/44100/stereo", enc));
    CHECK(!parse_encoding("xxx:s16/44100/stereo", enc));
    CHECK(!parse_encoding("-101:s16/44100/stereo", enc));
    CHECK(!parse_encoding("+101:s16/44100/stereo", enc));
    CHECK(!parse_encoding("101.2:s16/44100/stereo", enc));

    CHECK(parse_encoding("101:s16/44100/stereo", enc));
}

} // namespace rtp
} // namespace roc
