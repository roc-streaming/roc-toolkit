/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/depacketizer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = 20 * core::Second;

inline void write_zeros(sample_t* buf, size_t bufsz) {
    memset(buf, 0, bufsz * sizeof(sample_t));
}

inline void write_beep(sample_t* buf, size_t bufsz) {
    for (size_t n = 0; n < bufsz; n++) {
        buf[n] = (sample_t)std::sin(2 * M_PI / 44100 * 880 * n);
    }
}

} // namespace

Depacketizer::Depacketizer(packet::IReader& reader,
                           IFrameDecoder& payload_decoder,
                           const audio::SampleSpec& sample_spec,
                           bool beep)
    : reader_(reader)
    , payload_decoder_(payload_decoder)
    , sample_spec_(sample_spec)
    , timestamp_(0)
    , zero_samples_(0)
    , missing_samples_(0)
    , packet_samples_(0)
    , rate_limiter_(LogInterval)
    , first_packet_(true)
    , beep_(beep) {
    roc_log(LogDebug, "depacketizer: initializing: n_channels=%lu",
            (unsigned long)sample_spec_.num_channels());
}

bool Depacketizer::started() const {
    return !first_packet_;
}

packet::timestamp_t Depacketizer::timestamp() const {
    if (first_packet_) {
        return 0;
    }
    return timestamp_;
}

bool Depacketizer::read(Frame& frame) {
    read_frame_(frame);

    report_stats_();

    return true;
}

void Depacketizer::read_frame_(Frame& frame) {
    if (frame.size() % sample_spec_.num_channels() != 0) {
        roc_panic("depacketizer: unexpected frame size");
    }

    sample_t* buff_ptr = frame.data();
    sample_t* buff_end = frame.data() + frame.size();

    FrameInfo info;

    while (buff_ptr < buff_end) {
        buff_ptr = read_samples_(buff_ptr, buff_end, info);
    }

    roc_panic_if(buff_ptr != buff_end);

    set_frame_flags_(frame, info);
}

sample_t*
Depacketizer::read_samples_(sample_t* buff_ptr, sample_t* buff_end, FrameInfo& info) {
    update_packet_(info);

    if (packet_) {
        packet::timestamp_t next_timestamp = payload_decoder_.position();

        if (timestamp_ != next_timestamp) {
            roc_panic_if_not(packet::timestamp_lt(timestamp_, next_timestamp));

            const size_t mis_samples = sample_spec_.num_channels()
                * (size_t)packet::timestamp_diff(next_timestamp, timestamp_);

            const size_t max_samples = (size_t)(buff_end - buff_ptr);

            buff_ptr = read_missing_samples_(
                buff_ptr, buff_ptr + std::min(mis_samples, max_samples));
        }

        if (buff_ptr < buff_end) {
            sample_t* new_buff_ptr = read_packet_samples_(buff_ptr, buff_end);

            info.n_decoded_samples += size_t(new_buff_ptr - buff_ptr);

            buff_ptr = new_buff_ptr;
        }

        return buff_ptr;
    } else {
        return read_missing_samples_(buff_ptr, buff_end);
    }
}

sample_t* Depacketizer::read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t requested_samples =
        size_t(buff_end - buff_ptr) / sample_spec_.num_channels();

    const size_t decoded_samples = payload_decoder_.read(buff_ptr, requested_samples);

    timestamp_ += packet::timestamp_t(decoded_samples);
    packet_samples_ += decoded_samples;

    if (decoded_samples < requested_samples) {
        payload_decoder_.end();
        packet_ = NULL;
    }

    return (buff_ptr + decoded_samples * sample_spec_.num_channels());
}

sample_t* Depacketizer::read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t num_samples =
        (size_t)(buff_end - buff_ptr) / sample_spec_.num_channels();

    if (beep_) {
        write_beep(buff_ptr, num_samples * sample_spec_.num_channels());
    } else {
        write_zeros(buff_ptr, num_samples * sample_spec_.num_channels());
    }

    timestamp_ += packet::timestamp_t(num_samples);

    if (first_packet_) {
        zero_samples_ += num_samples;
    } else {
        missing_samples_ += num_samples;
    }

    return (buff_ptr + num_samples * sample_spec_.num_channels());
}

void Depacketizer::update_packet_(FrameInfo& info) {
    if (packet_) {
        return;
    }

    packet::timestamp_t pkt_timestamp = 0;
    unsigned n_dropped = 0;

    while ((packet_ = read_packet_())) {
        payload_decoder_.begin(packet_->rtp()->timestamp, packet_->rtp()->payload.data(),
                               packet_->rtp()->payload.size());

        pkt_timestamp = payload_decoder_.position();

        if (first_packet_) {
            break;
        }

        const packet::timestamp_t pkt_end = pkt_timestamp + payload_decoder_.available();

        if (packet::timestamp_lt(timestamp_, pkt_end)) {
            break;
        }

        roc_log(LogDebug, "depacketizer: dropping late packet: ts=%lu pkt_ts=%lu",
                (unsigned long)timestamp_, (unsigned long)pkt_timestamp);

        n_dropped++;

        payload_decoder_.end();
    }

    if (n_dropped != 0) {
        roc_log(LogDebug, "depacketizer: fetched=%d dropped=%u", (int)!!packet_,
                n_dropped);

        info.n_dropped_packets += n_dropped;
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

    if (packet::timestamp_lt(pkt_timestamp, timestamp_)) {
        const size_t diff_samples =
            (size_t)packet::timestamp_diff(timestamp_, pkt_timestamp);

        if (payload_decoder_.shift(diff_samples) != diff_samples) {
            roc_panic("depacketizer: can't shift packet");
        }
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

void Depacketizer::set_frame_flags_(Frame& frame, const FrameInfo& info) {
    unsigned flags = 0;

    if (info.n_decoded_samples != 0) {
        flags |= Frame::FlagNonblank;
    }

    if (info.n_decoded_samples < frame.size()) {
        flags |= Frame::FlagIncomplete;
    }

    if (info.n_dropped_packets != 0) {
        flags |= Frame::FlagDrops;
    }

    frame.set_flags(flags);
}

void Depacketizer::report_stats_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    const size_t total_samples = missing_samples_ + packet_samples_;
    const double loss_ratio =
        total_samples != 0 ? (double)missing_samples_ / total_samples : 0.;

    roc_log(LogDebug, "depacketizer: ts=%lu loss_ratio=%.5lf", (unsigned long)timestamp_,
            loss_ratio);
}

} // namespace audio
} // namespace roc
