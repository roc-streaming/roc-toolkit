/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/scoped_ptr.h"

namespace roc {
namespace audio {

namespace {

enum {
    Codec_PCM_int16_1ch,
    Codec_PCM_int16_2ch,

    NumCodecs
};

packet::channel_mask_t Codec_channels[NumCodecs] = {
    0x1,
    0x3
};

enum { MaxChans = 8, MaxBufSize = 1000 };

const double Epsilon = 0.00001;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);

sample_t nth_sample(uint8_t n) {
    return sample_t(n) / sample_t(1 << 8);
}

} // namespace

TEST_GROUP(encoder_decoder) {
    IFrameEncoder* new_encoder(size_t id) {
        switch (id) {
        case Codec_PCM_int16_1ch:
            return new (allocator) PcmEncoder(PCM_int16_1ch);

        case Codec_PCM_int16_2ch:
            return new (allocator) PcmEncoder(PCM_int16_2ch);

        default:
            FAIL("bad codec id");
        }

        return NULL;
    }

    IFrameDecoder* new_decoder(size_t id) {
        switch (id) {
        case Codec_PCM_int16_1ch:
            return new (allocator) PcmDecoder(PCM_int16_1ch);

        case Codec_PCM_int16_2ch:
            return new (allocator) PcmDecoder(PCM_int16_2ch);

        default:
            FAIL("bad codec id");
        }

        return NULL;
    }

    core::Slice<uint8_t> new_buffer(size_t buffer_size) {
        core::Slice<uint8_t> bp =
            new (byte_buffer_pool) core::Buffer<uint8_t>(byte_buffer_pool);
        CHECK(bp);

        bp.resize(buffer_size);

        return bp;
    }

    size_t fill_samples(sample_t* samples,
                        size_t pos,
                        size_t n_samples,
                        packet::channel_mask_t ch_mask) {
        const size_t n_chans = packet::num_channels(ch_mask);

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
                         packet::channel_mask_t ch_mask) {
        const size_t n_chans = packet::num_channels(ch_mask);

        for (size_t i = 0; i < n_samples; i++) {
            for (size_t j = 0; j < n_chans; j++) {
                sample_t actual = *samples++;
                sample_t expected = nth_sample(uint8_t(pos++));

                DOUBLES_EQUAL(expected, actual, Epsilon);
            }
        }

        return pos;
    }

    size_t check_zeros(const sample_t* samples, size_t pos, size_t n_samples) {
        for (size_t i = 0; i < n_samples; i++) {
            sample_t actual = *samples++;
            DOUBLES_EQUAL(0.0, actual, Epsilon);
            pos++;
        }

        return pos;
    }
};

TEST(encoder_decoder, one_frame) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read(decoder_samples, SamplesPerFrame,
                                           Codec_channels[n_codec]));

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end();

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());
    }
}

TEST(encoder_decoder, multiple_frames) {
    enum { NumFrames = 20, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

            encoder->begin(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 encoder->write(encoder_samples, SamplesPerFrame,
                                                Codec_channels[n_codec]));

            encoder->end();

            decoder->begin(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 decoder->read(decoder_samples, SamplesPerFrame,
                                               Codec_channels[n_codec]));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end();

            decoder_pos = check_samples(decoder_samples, decoder_pos, SamplesPerFrame,
                                        Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += SamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, incomplete_frames) {
    enum {
        NumFrames = 20,
        ExpectedSamplesPerFrame = 211,
        ActualSamplesPerFrame = 177
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp =
                new_buffer(encoder->encoded_size(ExpectedSamplesPerFrame));

            encoder->begin(bp.data(), bp.size());

            sample_t encoder_samples[ActualSamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos,
                                       ActualSamplesPerFrame, Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(ActualSamplesPerFrame,
                                 encoder->write(encoder_samples, ActualSamplesPerFrame,
                                                Codec_channels[n_codec]));

            encoder->end();

            bp.resize(encoder->encoded_size(ActualSamplesPerFrame));

            decoder->begin(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(ActualSamplesPerFrame, decoder->available());

            sample_t decoder_samples[ActualSamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(ActualSamplesPerFrame,
                                 decoder->read(decoder_samples, ExpectedSamplesPerFrame,
                                               Codec_channels[n_codec]));

            UNSIGNED_LONGS_EQUAL(ts + ActualSamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end();

            decoder_pos = check_samples(decoder_samples, decoder_pos, ActualSamplesPerFrame,
                                        Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);

            ts += ActualSamplesPerFrame;
        }
    }
}

TEST(encoder_decoder, shifted_frames) {
    enum { NumFrames = 20, SamplesPerFrame = 177, Shift = 55 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

            encoder->begin(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 encoder->write(encoder_samples, SamplesPerFrame,
                                                Codec_channels[n_codec]));

            encoder->end();

            decoder->begin(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            UNSIGNED_LONGS_EQUAL(Shift, decoder->shift(Shift));

            UNSIGNED_LONGS_EQUAL(ts + Shift, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame - Shift, decoder->available());

            decoder_pos += Shift * packet::num_channels(Codec_channels[n_codec]);

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(
                SamplesPerFrame - Shift,
                decoder->read(decoder_samples, SamplesPerFrame, Codec_channels[n_codec]));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end();

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
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::timestamp_t ts = 100500;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumFrames; n++) {
            core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

            encoder->begin(bp.data(), bp.size());

            sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerFrame,
                                       Codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 encoder->write(encoder_samples, SamplesPerFrame,
                                                Codec_channels[n_codec]));

            encoder->end();

            if (n % SkipEvery == 0) {
                ts += SamplesPerFrame;
                decoder_pos +=
                    SamplesPerFrame * packet::num_channels(Codec_channels[n_codec]);
                continue;
            }

            decoder->begin(ts, bp.data(), bp.size());

            UNSIGNED_LONGS_EQUAL(ts, decoder->position());
            UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                                 decoder->read(decoder_samples, SamplesPerFrame,
                                               Codec_channels[n_codec]));

            UNSIGNED_LONGS_EQUAL(ts + SamplesPerFrame, decoder->position());
            UNSIGNED_LONGS_EQUAL(0, decoder->available());

            decoder->end();

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
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(
            FirstPart,
            encoder->write(encoder_samples, FirstPart, Codec_channels[n_codec]));

        UNSIGNED_LONGS_EQUAL(
            SecondPart,
            encoder->write(encoder_samples
                               + FirstPart
                                   * packet::num_channels(Codec_channels[n_codec]),
                           SecondPart, Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read(decoder_samples, SamplesPerFrame,
                                           Codec_channels[n_codec]));

        decoder->end();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, write_too_much) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[(SamplesPerFrame + 20) * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame + 20,
                     Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame + 20,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read(decoder_samples, SamplesPerFrame,
                                           Codec_channels[n_codec]));

        decoder->end();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, write_channel_mask) {
    enum {
        Timestamp = 100500,
        FirstPart = 33,
        SecondPart = 44,
        FirstPartChans = 0xff,
        SecondPartChans = 0x1,
        SamplesPerFrame = FirstPart + SecondPart
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        size_t encoder_pos = 0;

        {
            sample_t encoder_samples[FirstPart * MaxChans] = {};
            encoder_pos =
                fill_samples(encoder_samples, encoder_pos, FirstPart, FirstPartChans);

            UNSIGNED_LONGS_EQUAL(
                FirstPart, encoder->write(encoder_samples, FirstPart, FirstPartChans));
        }

        {
            sample_t encoder_samples[SecondPart * MaxChans] = {};
            encoder_pos =
                fill_samples(encoder_samples, encoder_pos, SecondPart, SecondPartChans);

            UNSIGNED_LONGS_EQUAL(
                SecondPart, encoder->write(encoder_samples, SecondPart, SecondPartChans));
        }

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[SamplesPerFrame * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read(decoder_samples, SamplesPerFrame,
                                           Codec_channels[n_codec]));

        decoder->end();

        size_t actual_pos = 0;
        size_t expected_pos = 0;

        for (size_t i = 0; i < FirstPart; i++) {
            for (size_t j = 0; j < packet::num_channels(FirstPartChans); j++) {
                if (Codec_channels[n_codec] & (1 << j)) {
                    sample_t actual = decoder_samples[actual_pos++];
                    sample_t expected = nth_sample(uint8_t(expected_pos));

                    DOUBLES_EQUAL(expected, actual, Epsilon);
                }

                expected_pos++;
            }
        }

        for (size_t i = FirstPart; i < SamplesPerFrame; i++) {
            for (size_t j = 0; j < packet::num_channels(Codec_channels[n_codec]);
                 j++) {
                sample_t actual = decoder_samples[actual_pos++];
                sample_t expected = 0;

                if (SecondPartChans & (1 << j)) {
                    expected = nth_sample(uint8_t(expected_pos++));
                }

                DOUBLES_EQUAL(expected, actual, Epsilon);
            }
        }
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
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        size_t encoder_pos = fill_samples(encoder_samples, 0, SamplesPerFrame,
                                          Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        size_t decoder_pos = 0;

        {
            sample_t decoder_samples[FirstPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(FirstPart,
                                 decoder->read(decoder_samples, FirstPart,
                                               Codec_channels[n_codec]));

            decoder_pos = check_samples(decoder_samples, decoder_pos, FirstPart,
                                        Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart, decoder->available());

        {
            sample_t decoder_samples[SecondPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(SecondPart,
                                 decoder->read(decoder_samples, SecondPart,
                                               Codec_channels[n_codec]));

            decoder_pos = check_samples(decoder_samples, decoder_pos, SecondPart,
                                        Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end();

        UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);
    }
}

TEST(encoder_decoder, read_too_much) {
    enum { Timestamp = 100500, SamplesPerFrame = 177 };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        sample_t decoder_samples[(SamplesPerFrame + 20) * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             decoder->read(decoder_samples, SamplesPerFrame + 20,
                                           Codec_channels[n_codec]));

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end();

        check_samples(decoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);
    }
}

TEST(encoder_decoder, read_channel_mask) {
    enum {
        Timestamp = 100500,
        FirstPart = 33,
        SecondPart = 44,
        FirstPartChans = 0xff,
        SecondPartChans = 0x1,
        SamplesPerFrame = FirstPart + SecondPart
    };

    for (size_t n_codec = 0; n_codec < NumCodecs; n_codec++) {
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        size_t encoder_pos = fill_samples(encoder_samples, 0, SamplesPerFrame,
                                          Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        size_t decoder_pos = 0;

        {
            sample_t decoder_samples[FirstPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(
                FirstPart, decoder->read(decoder_samples, FirstPart, FirstPartChans));

            size_t actual_pos = 0;

            for (size_t i = 0; i < FirstPart; i++) {
                for (size_t j = 0; j < packet::num_channels(FirstPartChans); j++) {
                    sample_t actual = decoder_samples[actual_pos++];
                    sample_t expected = 0;

                    if (Codec_channels[n_codec] & (1 << j)) {
                        expected = nth_sample(uint8_t(decoder_pos++));
                    }

                    DOUBLES_EQUAL(expected, actual, Epsilon);
                }
            }
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart, decoder->available());

        {
            sample_t decoder_samples[SecondPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(
                SecondPart, decoder->read(decoder_samples, SecondPart, SecondPartChans));

            size_t actual_pos = 0;

            for (size_t i = FirstPart; i < SamplesPerFrame; i++) {
                for (size_t j = 0; j < packet::num_channels(Codec_channels[n_codec]);
                     j++) {
                    if (SecondPartChans & (1 << j)) {
                        sample_t actual = decoder_samples[actual_pos++];
                        sample_t expected = nth_sample(uint8_t(decoder_pos));

                        DOUBLES_EQUAL(expected, actual, Epsilon);
                    }

                    decoder_pos++;
                }
            }
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end();

        UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);
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
        core::ScopedPtr<IFrameEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::ScopedPtr<IFrameDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        core::Slice<uint8_t> bp = new_buffer(encoder->encoded_size(SamplesPerFrame));

        encoder->begin(bp.data(), bp.size());

        sample_t encoder_samples[SamplesPerFrame * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerFrame, Codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerFrame,
                             encoder->write(encoder_samples, SamplesPerFrame,
                                            Codec_channels[n_codec]));

        encoder->end();

        decoder->begin(Timestamp, bp.data(), bp.size());

        UNSIGNED_LONGS_EQUAL(Timestamp, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame, decoder->available());

        UNSIGNED_LONGS_EQUAL(FirstPart, decoder->shift(FirstPart));

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart, decoder->available());

        {
            sample_t decoder_samples[SecondPart * MaxChans];

            UNSIGNED_LONGS_EQUAL(SecondPart,
                                 decoder->read(decoder_samples, SecondPart,
                                               Codec_channels[n_codec]));

            check_samples(decoder_samples,
                          FirstPart * packet::num_channels(Codec_channels[n_codec]),
                          SecondPart, Codec_channels[n_codec]);
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + FirstPart + SecondPart, decoder->position());
        UNSIGNED_LONGS_EQUAL(SamplesPerFrame - FirstPart - SecondPart,
                             decoder->available());

        UNSIGNED_LONGS_EQUAL(ThirdPart, decoder->shift(ThirdPart + 20));

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        {
            sample_t decoder_samples[SamplesPerFrame * MaxChans];

            UNSIGNED_LONGS_EQUAL(0,
                                 decoder->read(decoder_samples, SamplesPerFrame,
                                               Codec_channels[n_codec]));
        }

        UNSIGNED_LONGS_EQUAL(Timestamp + SamplesPerFrame, decoder->position());
        UNSIGNED_LONGS_EQUAL(0, decoder->available());

        decoder->end();
    }
}

} // namespace audio
} // namespace roc
