/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/pcm_subformat.h"
#include "roc_core/heap_arena.h"
#include "roc_core/scoped_ptr.h"

namespace roc {
namespace audio {

namespace {

enum {
    Codec_PCM_SInt16_1ch,
    Codec_PCM_SInt16_2ch,
    Codec_PCM_SInt24_1ch,
    Codec_PCM_SInt24_2ch,

    NumCodecs
};

const ChannelMask Codec_channels[NumCodecs] = {
    ChanMask_Surround_Mono,
    ChanMask_Surround_Stereo,
    ChanMask_Surround_Mono,
    ChanMask_Surround_Stereo,
};

enum { SampleRate = 44100, MaxChans = 8, MaxBufSize = 2000 };

const double Epsilon = 0.00001;

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBufSize);

sample_t nth_sample(uint8_t n) {
    return sample_t(n) / sample_t(1 << 8);
}

IFrameEncoder* new_encoder(size_t id) {
    switch (id) {
    case Codec_PCM_SInt16_1ch:
        return new (arena)
            PcmEncoder(SampleSpec(SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Mono),
                       arena);

    case Codec_PCM_SInt16_2ch:
        return new (arena)
            PcmEncoder(SampleSpec(SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Stereo),
                       arena);

    case Codec_PCM_SInt24_1ch:
        return new (arena)
            PcmEncoder(SampleSpec(SampleRate, PcmSubformat_SInt24_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Mono),
                       arena);

    case Codec_PCM_SInt24_2ch:
        return new (arena)
            PcmEncoder(SampleSpec(SampleRate, PcmSubformat_SInt24_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Stereo),
                       arena);

    default:
        FAIL("bad codec id");
    }

    return NULL;
}

IFrameDecoder* new_decoder(size_t id) {
    switch (id) {
    case Codec_PCM_SInt16_1ch:
        return new (arena)
            PcmDecoder(SampleSpec(SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Mono),
                       arena);

    case Codec_PCM_SInt16_2ch:
        return new (arena)
            PcmDecoder(SampleSpec(SampleRate, PcmSubformat_SInt16_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Stereo),
                       arena);

    case Codec_PCM_SInt24_1ch:
        return new (arena)
            PcmDecoder(SampleSpec(SampleRate, PcmSubformat_SInt24_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Mono),
                       arena);

    case Codec_PCM_SInt24_2ch:
        return new (arena)
            PcmDecoder(SampleSpec(SampleRate, PcmSubformat_SInt24_Be, ChanLayout_Surround,
                                  ChanOrder_Smpte, ChanMask_Surround_Stereo),
                       arena);

    default:
        FAIL("bad codec id");
    }

    return NULL;
}

core::Slice<uint8_t> new_buffer(size_t buffer_size) {
    core::Slice<uint8_t> bp = frame_factory.new_byte_buffer();
    CHECK(bp);

    bp.reslice(0, buffer_size);

    return bp;
}

size_t num_channels(ChannelMask ch_mask) {
    size_t n_ch = 0;
    for (; ch_mask != 0; ch_mask >>= 1) {
        if (ch_mask & 1) {
            n_ch++;
        }
    }
    return n_ch;
}

size_t
fill_samples(sample_t* samples, size_t pos, size_t n_samples, ChannelMask ch_mask) {
    const size_t n_chans = num_channels(ch_mask);

    for (size_t i = 0; i < n_samples; i++) {
        for (size_t j = 0; j < n_chans; j++) {
            *samples++ = nth_sample(uint8_t(pos++));
        }
    }

    return pos;
}

size_t check_samples(const sample_t* samples,
                     size_t pos,
                     size_t n_samples,
                     ChannelMask ch_mask) {
    const size_t n_chans = num_channels(ch_mask);

    for (size_t i = 0; i < n_samples; i++) {
        for (size_t j = 0; j < n_chans; j++) {
            sample_t actual = *samples++;
            sample_t expected = nth_sample(uint8_t(pos++));

            DOUBLES_EQUAL(expected, actual, Epsilon);
        }
    }

    return pos;
}

} // namespace

TEST_GROUP(encoder_decoder) {};

TEST(encoder_decoder, one_frame) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write_samples(encoder_samples, SamplesPerFrame));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read_samples(decoder_samples, SamplesPerFrame));

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end_frame();

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());
    }
}

TEST(encoder_decoder, multiple_frames) {
    enum { NumFrames = 20, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        packet::stream_timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp =
                new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

            encoder->begin_frame(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(
                SamplesPerFrame,
                encoder->write_samples(encoder_samples, SamplesPerFrame));

            encoder->end_frame();

            decoder->begin_frame(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 decoder->read_samples(decoder_samples, SamplesPerFrame));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end_frame();

            decoder_pos = check_samples(decoder_samples, decoder_pos, SamplesPerFrame,
                                        Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += SamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, incomplete_frames) {
    enum { NumFrames = 20, ExpectedSamplesPerFrame = 211, ActualSamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        packet::stream_timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp =
                new_buffer(encoder->encoded_byte_count(ExpectedSamplesPerFrame));

            encoder->begin_frame(bp.data(), bp.size());

            sample_t encoder_samples[ActualSamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos,
                                       ActualSamplesPerFrame, Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(
                ActualSamplesPerFrame,
                encoder->write_samples(encoder_samples, ActualSamplesPerFrame));

            encoder->end_frame();

            bp.reslice(0, encoder->encoded_byte_count(ActualSamplesPerFrame));

            decoder->begin_frame(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(ActualSamplesPerFrame, decoder->available());

            sample_t decoder_samples[ActualSamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(
                ActualSamplesPerFrame,
                decoder->read_samples(decoder_samples, ExpectedSamplesPerFrame));

            UNSIGNED_LONGS_EQUAL(ts + ActualSamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end_frame();

            decoder_pos = check_samples(decoder_samples, decoder_pos,
                                        ActualSamplesPerFrame, Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += ActualSamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, shifted_frames) {
    enum { NumFrames = 20, SamplesPerFrame = 177, Shift = 55 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        packet::stream_timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp =
                new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

            encoder->begin_frame(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(
                SamplesPerFrame,
                encoder->write_samples(encoder_samples, SamplesPerFrame));

            encoder->end_frame();

            decoder->begin_frame(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            UNSIGNED_LONGS_EQUAL(Shift, decoder->drop_samples(Shift));

            UNSIGNED_LONGS_EQUAL(ts + Shift, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame - Shift, decoder->available());

            decoder_pos += Shift * num_channels(Codec_channels[n_codec]);

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame - Shift,
                                 decoder->read_samples(decoder_samples, SamplesPerFrame));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end_frame();

            decoder_pos = check_samples(decoder_samples, decoder_pos,
                                        SamplesPerFrame - Shift, Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += SamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, skipped_frames) {
    enum { NumFrames = 20, SkipEvery = 3, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        packet::stream_timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp =
                new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

            encoder->begin_frame(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(
                SamplesPerFrame,
                encoder->write_samples(encoder_samples, SamplesPerFrame));

            encoder->end_frame();

            if (n % SkipEvery == 0) {
                ts += SamplesPerFrame;
                decoder_pos += SamplesPerFrame * num_channels(Codec_channels[n_codec]);
                continue;
            }

            decoder->begin_frame(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 decoder->read_samples(decoder_samples, SamplesPerFrame));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end_frame();

            decoder_pos = check_samples(decoder_samples, decoder_pos, SamplesPerFrame,
                                        Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += SamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, write_incrementally) {
    enum {
        Timestamp = 100500,
        FirstPart = 33,
        SecondPart = 44,
        SamplesPerFrame = FirstPart + SecondPart
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(FirstPart,
                             encoder->write_samples(encoder_samples, FirstPart));

        UNSIGNED_LONGS_EQUAL(
            SecondPart,
            encoder->write_samples(
                encoder_samples + FirstPart * num_channels(Codec_channels[n_codec]),
                SecondPart));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read_samples(decoder_samples, SamplesPerFrame));

        decoder->end_frame();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, write_too_much) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[(SamplesPerFrame + 20) * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame + 20, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(
            SamplesPerFrame,
            encoder->write_samples(encoder_samples, SamplesPerFrame + 20));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read_samples(decoder_samples, SamplesPerFrame));

        decoder->end_frame();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, read_incrementally) {
    enum {
        Timestamp = 100500,
        FirstPart = 33,
        SecondPart = 44,
        SamplesPerFrame = FirstPart + SecondPart
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        size_t encoder_pos =
            fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write_samples(encoder_samples, SamplesPerFrame));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        size_t decoder_pos = 0;

        {
            sample_t decoder_samples[FirstPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(FirstPart,
                                 decoder->read_samples(decoder_samples, FirstPart));

            decoder_pos = check_samples(decoder_samples, decoder_pos, FirstPart,
                                        Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart, decoder->available());

        {
            sample_t decoder_samples[SecondPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(SecondPart,
                                 decoder->read_samples(decoder_samples, SecondPart));

            decoder_pos = check_samples(decoder_samples, decoder_pos, SecondPart,
                                        Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end_frame();

        UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);
    }
}

TEST(encoder_decoder, read_too_much) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write_samples(encoder_samples, SamplesPerFrame));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[(SamplesPerFrame + 20) * MaxChans];

        UNSIGNED_LONGS_EQUAL(
            SamplesPerFrame,
            decoder->read_samples(decoder_samples, SamplesPerFrame + 20));

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end_frame();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, shift_incrementally) {
    enum {
        Timestamp = 100500,
        FirstPart = 33,
        SecondPart = 44,
        ThirdPart = 11,
        SamplesPerFrame = FirstPart + SecondPart + ThirdPart
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec));
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec));
        CHECK(decoder);

        core::Slice<uint8_t> bp =
            new_buffer(encoder->encoded_byte_count(SamplesPerFrame));

        encoder->begin_frame(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write_samples(encoder_samples, SamplesPerFrame));

        encoder->end_frame();

        decoder->begin_frame(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        UNSIGNED_LONGS_EQUAL(FirstPart, decoder->drop_samples(FirstPart));

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart, decoder->available());

        {
            sample_t decoder_samples[SecondPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(SecondPart,
                                 decoder->read_samples(decoder_samples, SecondPart));

            check_samples(decoder_samples,
                          FirstPart * num_channels(Codec_channels[n_codec]), SecondPart,
                          Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart + SecondPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart - SecondPart,
                             decoder->available());

        UNSIGNED_LONGS_EQUAL(ThirdPart, decoder->drop_samples(ThirdPart + 20));

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        {
            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(0,
                                 decoder->read_samples(decoder_samples, SamplesPerFrame));
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end_frame();
    }
}

} // namespace audio
} // namespace roc
