/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/log.h"

#include "roc_rtp/parser.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace test {

using namespace packet;

namespace {

enum { MaxCh = 2, NumSamples = 237, Guard = 7 };

enum { Rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE };

const double Epsilon = 0.0001;

} // namespace

TEST_GROUP(audio_packet) {
    rtp::Composer composer;
    rtp::Parser parser;

    IAudioPacketPtr compose() {
        IPacketPtr packet = composer.compose(IAudioPacket::Type);
        CHECK(packet);
        CHECK(packet->type() == IAudioPacket::Type);
        return static_cast<IAudioPacket*>(packet.get());
    }

    IAudioPacketConstPtr parse(const core::IByteBufferConstSlice& buff) {
        IPacketConstPtr packet = parser.parse(buff);
        CHECK(packet);
        CHECK(packet->type() == IAudioPacket::Type);
        return static_cast<const IAudioPacket*>(packet.get());
    }

    sample_t make_sample(size_t n) {
        return 0.001f * n;
    }

    void check_sample(sample_t s, size_t n, double epsilon = Epsilon) {
        DOUBLES_EQUAL(0.001f * n, s, epsilon);
    }

    void write_samples(const IAudioPacketPtr& packet, size_t num_ch, size_t num_samples) {
        sample_t samples[NumSamples * MaxCh] = {};

        for (size_t ns = 0; ns < num_samples * num_ch; ns++) {
            samples[ns] = make_sample(ns);
        }

        channel_mask_t ch_mask = (1 << num_ch) - 1;

        packet->write_samples(ch_mask, 0, samples, num_samples);
    }
};

TEST(audio_packet, compose_empty) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();

        LONGS_EQUAL(0, p->source());
        LONGS_EQUAL(0, p->seqnum());
        CHECK(!p->marker());
        LONGS_EQUAL(0, p->timestamp());

        p->set_size(ch_mask, 0, Rate);

        LONGS_EQUAL(ch_mask, p->channels());
        LONGS_EQUAL(0, p->num_samples());
        LONGS_EQUAL(Rate, p->rate());

        sample_t samples[MaxCh] = {};
        LONGS_EQUAL(0, p->read_samples(ch_mask, 0, samples, 1));
    }
}

TEST(audio_packet, compose_full) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();

        p->set_source(3456776543u);
        p->set_seqnum(12345);
        p->set_marker(true);
        p->set_timestamp(123456789);

        LONGS_EQUAL(3456776543u, p->source());
        LONGS_EQUAL(12345, p->seqnum());
        CHECK(p->marker());
        LONGS_EQUAL(123456789, p->timestamp());

        p->set_size(ch_mask, NumSamples, Rate);

        LONGS_EQUAL(ch_mask, p->channels());
        LONGS_EQUAL(NumSamples, p->num_samples());
        LONGS_EQUAL(Rate, p->rate());

        write_samples(p, num_ch, NumSamples);

        {
            sample_t samples[NumSamples * MaxCh] = {};
            LONGS_EQUAL(NumSamples, p->read_samples(ch_mask, 0, samples, NumSamples));

            for (size_t ns = 0; ns < NumSamples * num_ch; ns++) {
                check_sample(samples[ns], ns);
            }
        }
    }
}

TEST(audio_packet, compose_parse) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p1 = compose();

        p1->set_source(3456776543u);
        p1->set_seqnum(12345);
        p1->set_marker(true);
        p1->set_timestamp(123456789);

        p1->set_size(ch_mask, NumSamples, Rate);

        write_samples(p1, num_ch, NumSamples);

        IAudioPacketConstPtr p2 = parse(p1->raw_data());

        LONGS_EQUAL(3456776543u, p2->source());
        LONGS_EQUAL(12345, p2->seqnum());
        CHECK(p2->marker());
        LONGS_EQUAL(123456789, p2->timestamp());

        LONGS_EQUAL(ch_mask, p2->channels());
        LONGS_EQUAL(NumSamples, p2->num_samples());
        LONGS_EQUAL(Rate, p2->rate());

        {
            sample_t samples[NumSamples * MaxCh] = {};
            LONGS_EQUAL(NumSamples, p2->read_samples(ch_mask, 0, samples, NumSamples));

            for (size_t ns = 0; ns < NumSamples * num_ch; ns++) {
                check_sample(samples[ns], ns);
            }
        }
    }
}

TEST(audio_packet, read_one_channel) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();
        p->set_size(ch_mask, NumSamples, Rate);

        write_samples(p, num_ch, NumSamples);

        for (size_t ch = 0; ch < num_ch; ch++) {
            sample_t samples[NumSamples] = {};
            LONGS_EQUAL(NumSamples, p->read_samples((1 << ch), 0, samples, NumSamples));

            for (size_t ns = 0; ns < NumSamples; ns++) {
                check_sample(samples[ns], ns * num_ch + ch);
            }
        }
    }
}

TEST(audio_packet, read_offset_and_length) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();
        p->set_size(ch_mask, NumSamples, Rate);

        write_samples(p, num_ch, NumSamples);

        for (size_t ch = 0; ch < num_ch; ch++) {
            for (size_t ns = 0; ns < NumSamples; ns++) {
                sample_t sample = 0;
                LONGS_EQUAL(1, p->read_samples((1 << ch), ns, &sample, 1));
                check_sample(sample, ns * num_ch + ch);
            }
        }

        {
            sample_t samples[10 * MaxCh + Guard] = {};
            LONGS_EQUAL(10, p->read_samples(ch_mask, 0, samples, 10));

            for (size_t ns = 0; ns < 10 * num_ch; ns++) {
                check_sample(samples[ns], ns);
            }

            for (size_t ns = 10 * num_ch; ns < 10 * num_ch + Guard; ns++) {
                check_sample(samples[ns], 0, 0);
            }
        }

        {
            sample_t samples[10 * MaxCh + Guard] = {};
            LONGS_EQUAL(10, p->read_samples(ch_mask, NumSamples - 10, samples, 10));

            for (size_t ns = 0; ns < 10 * num_ch; ns++) {
                check_sample(samples[ns], (NumSamples - 10) * num_ch + ns);
            }

            for (size_t ns = 10 * num_ch; ns < 10 * num_ch + Guard; ns++) {
                check_sample(samples[ns], 0, 0);
            }
        }
    }
}

TEST(audio_packet, read_more_than_size) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();
        p->set_size(ch_mask, NumSamples, Rate);

        write_samples(p, num_ch, NumSamples);

        for (size_t ch = 0; ch < num_ch; ch++) {
            sample_t samples[NumSamples] = {};

            for (size_t off = 0; off < NumSamples; off++) {
                LONGS_EQUAL(NumSamples - off,
                            p->read_samples((1 << ch), off, samples, NumSamples));

                for (size_t ns = off; ns < NumSamples; ns++) {
                    check_sample(samples[ns - off], ns * num_ch + ch);
                }
            }
        }

        {
            sample_t samples[NumSamples * MaxCh] = {};

            for (size_t off = 0; off < NumSamples; off++) {
                LONGS_EQUAL(NumSamples - off,
                            p->read_samples(ch_mask, off, samples, NumSamples));

                for (size_t ns = off * num_ch; ns < NumSamples * num_ch; ns++) {
                    check_sample(samples[ns - off * num_ch], ns);
                }
            }
        }
    }
}

TEST(audio_packet, write_one_channel) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();
        p->set_size(ch_mask, NumSamples, Rate);

        for (size_t ch = 0; ch < num_ch; ch++) {
            {
                sample_t samples[NumSamples] = {};

                for (size_t ns = 0; ns < NumSamples; ns++) {
                    samples[ns] = make_sample(ch * NumSamples + ns);
                }

                p->write_samples((1 << ch), 0, samples, NumSamples);
            }

            for (size_t rch = 0; rch < num_ch; rch++) {
                sample_t samples[NumSamples * MaxCh] = {};
                LONGS_EQUAL(NumSamples,
                            p->read_samples((1 << rch), 0, samples, NumSamples));

                for (size_t ns = 0; ns < NumSamples; ns++) {
                    if (rch <= ch) {
                        check_sample(samples[ns], rch * NumSamples + ns);
                    } else {
                        check_sample(samples[ns], 0, 0);
                    }
                }
            }
        }

        sample_t samples[NumSamples * MaxCh] = {};
        LONGS_EQUAL(NumSamples, p->read_samples(ch_mask, 0, samples, NumSamples));

        for (size_t ch = 0; ch < num_ch; ch++) {
            for (size_t ns = 0; ns < NumSamples; ns++) {
                check_sample(samples[ch + num_ch * ns], ch * NumSamples + ns);
            }
        }
    }
}

TEST(audio_packet, write_offset_and_length) {
    for (size_t num_ch = 1; num_ch <= MaxCh; num_ch++) {
        const channel_mask_t ch_mask = (1 << num_ch) - 1;

        IAudioPacketPtr p = compose();
        p->set_size(ch_mask, NumSamples, Rate);

        {
            sample_t samples[10 * MaxCh] = {};

            for (size_t ns = 0; ns < 10 * num_ch; ns++) {
                samples[ns] = make_sample(ns);
            }

            p->write_samples(ch_mask, 0, samples, 10);
        }

        for (size_t ch = 0; ch < num_ch; ch++) {
            sample_t samples[10] = {};

            for (size_t ns = 0; ns < 10; ns++) {
                samples[ns] = make_sample(ch * 10 + ns);
            }

            p->write_samples((1 << ch), NumSamples - 10, samples, 10);
        }

        sample_t samples[NumSamples * MaxCh] = {};
        LONGS_EQUAL(NumSamples, p->read_samples(ch_mask, 0, samples, NumSamples));

        for (size_t ns = 0; ns < 10 * num_ch; ns++) {
            check_sample(samples[ns], ns);
        }

        for (size_t ns = 10 * num_ch; ns < (NumSamples - 10) * num_ch; ns++) {
            check_sample(samples[ns], 0, 0);
        }

        for (size_t ch = 0; ch < num_ch; ch++) {
            for (size_t ns = 0; ns < 10; ns++) {
                check_sample(samples[(NumSamples - 10 + ns) * num_ch + ch], ch * 10 + ns);
            }
        }
    }
}

} // namespace test
} // namespace roc
