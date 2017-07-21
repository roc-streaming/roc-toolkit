/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/depacketizer.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogRate = 20000000000;

inline void write_zeros(sample_t* buf, size_t bufsz) {
    memset(buf, 0, bufsz * sizeof(sample_t));
}

inline void write_beep(sample_t* buf, size_t bufsz) {
    for (size_t n = 0; n < bufsz; n++) {
        buf[n] = (sample_t)sin(2 * M_PI / 44100 * 880 * n);
    }
}

} // namespace

Depacketizer::Depacketizer(packet::IReader& reader,
                           IDecoder& decoder,
                           packet::channel_mask_t channels,
                           bool beep)
    : reader_(reader)
    , decoder_(decoder)
    , channels_(channels)
    , num_channels_(packet::num_channels(channels))
    , packet_pos_(0)
    , timestamp_(0)
    , zero_samples_(0)
    , missing_samples_(0)
    , packet_samples_(0)
    , rate_limiter_(LogRate)
    , first_packet_(true)
    , beep_(beep) {
}

void Depacketizer::read(Frame& frame) {
    if (!frame.samples) {
        roc_panic("depacketizer: unexpected null slice");
    }

    if (frame.samples.size() % num_channels_ != 0) {
        roc_panic("depacketizer: unexpected frame size");
    }

    sample_t* buff_ptr = frame.samples.data();
    sample_t* buff_end = frame.samples.data() + frame.samples.size();

    while (buff_ptr < buff_end) {
        buff_ptr = read_samples_(buff_ptr, buff_end);
    }

    roc_panic_if(buff_ptr != buff_end);

    if (rate_limiter_.allow()) {
        const double total_samples = missing_samples_ + packet_samples_;

        roc_log(LogDebug, "depacketizer: ts=%lu loss_ratio=%.5lf",
                (unsigned long)timestamp_,
                total_samples != 0.0 ? missing_samples_ / total_samples : 0.0);
    }
}

sample_t* Depacketizer::read_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    update_packet_();

    if (packet_) {
        packet::timestamp_t next_timestamp = (packet_->rtp()->timestamp + packet_pos_);

        if (timestamp_ != next_timestamp) {
            roc_panic_if_not(
                ROC_UNSIGNED_LT(packet::signed_timestamp_t, timestamp_, next_timestamp));

            size_t mis_samples =
                num_channels_ * (size_t)ROC_UNSIGNED_SUB(packet::signed_timestamp_t,
                                                         next_timestamp, timestamp_);

            size_t max_samples = (size_t)(buff_end - buff_ptr);

            buff_ptr = read_missing_samples_(
                buff_ptr, buff_ptr + ROC_MIN(mis_samples, max_samples));
        }

        if (buff_ptr < buff_end) {
            buff_ptr = read_packet_samples_(buff_ptr, buff_end);
        }

        return buff_ptr;
    } else {
        return read_missing_samples_(buff_ptr, buff_end);
    }
}

sample_t* Depacketizer::read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t max_samples = (size_t)(buff_end - buff_ptr) / num_channels_;

    const size_t num_samples =
        decoder_.read_samples(*packet_, packet_pos_, buff_ptr, max_samples, channels_);

    timestamp_ += packet::timestamp_t(num_samples);
    packet_pos_ += packet::timestamp_t(num_samples);
    packet_samples_ += num_samples;

    if (num_samples == 0) {
        packet_.reset();
    }

    return (buff_ptr + num_samples * num_channels_);
}

sample_t* Depacketizer::read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    size_t num_samples = (size_t)(buff_end - buff_ptr) / num_channels_;

    if (beep_) {
        write_beep(buff_ptr, num_samples * num_channels_);
    } else {
        write_zeros(buff_ptr, num_samples * num_channels_);
    }

    timestamp_ += packet::timestamp_t(num_samples);

    if (first_packet_) {
        zero_samples_ += num_samples;
    } else {
        missing_samples_ += num_samples;
    }

    return (buff_ptr + num_samples * num_channels_);
}

void Depacketizer::update_packet_() {
    if (packet_) {
        return;
    }

    packet::timestamp_t pkt_timestamp = 0;
    unsigned n_dropped = 0;

    while ((packet_ = read_packet_())) {
        pkt_timestamp = packet_->rtp()->timestamp;

        if (first_packet_) {
            break;
        }

        const packet::timestamp_t pkt_end = pkt_timestamp + packet_->rtp()->duration;

        if (ROC_UNSIGNED_LT(packet::signed_timestamp_t, timestamp_, pkt_end)) {
            break;
        }

        roc_log(LogDebug, "depacketizer: dropping late packet: ts=%lu pkt_ts=%lu",
                (unsigned long)timestamp_, (unsigned long)pkt_timestamp);

        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogInfo, "depacketizer: fetched=%d dropped=%u", (int)!!packet_,
                n_dropped);
    }

    if (!packet_) {
        return;
    }

    if (first_packet_) {
        roc_log(LogDebug, "depacketizer: got first packet: zero_samples=%lu",
                (unsigned long)zero_samples_);

        timestamp_ = pkt_timestamp;
        first_packet_ = false;
    }

    if (ROC_UNSIGNED_LT(packet::signed_timestamp_t, pkt_timestamp, timestamp_)) {
        packet_pos_ = (packet::timestamp_t)ROC_UNSIGNED_SUB(packet::signed_timestamp_t,
                                                            timestamp_, pkt_timestamp);
    } else {
        packet_pos_ = 0;
    }
}

packet::PacketPtr Depacketizer::read_packet_() {
    packet::PacketPtr pp = reader_.read();
    if (!pp) {
        return NULL;
    }

    if (!pp->rtp()) {
        roc_panic("depacketizer: unexpected non-rtp packet");
    }

    return pp;
}

} // namespace audio
} // namespace roc
