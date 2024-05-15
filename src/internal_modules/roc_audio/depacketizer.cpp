/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/depacketizer.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_status/code_to_str.h"

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
                           const SampleSpec& sample_spec,
                           bool beep)
    : reader_(reader)
    , payload_decoder_(payload_decoder)
    , sample_spec_(sample_spec)
    , stream_ts_(0)
    , next_capture_ts_(0)
    , valid_capture_ts_(false)
    , zero_samples_(0)
    , missing_samples_(0)
    , packet_samples_(0)
    , rate_limiter_(LogInterval)
    , beep_(beep)
    , first_packet_(true)
    , valid_(false) {
    roc_panic_if_msg(!sample_spec_.is_valid() || !sample_spec_.is_raw(),
                     "depacketizer: required valid sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    roc_log(LogDebug, "depacketizer: initializing: n_channels=%lu",
            (unsigned long)sample_spec_.num_channels());

    valid_ = true;
}

bool Depacketizer::is_valid() const {
    return valid_;
}

bool Depacketizer::is_started() const {
    return !first_packet_;
}

packet::stream_timestamp_t Depacketizer::next_timestamp() const {
    if (first_packet_) {
        return 0;
    }
    return stream_ts_;
}

bool Depacketizer::read(Frame& frame) {
    read_frame_(frame);

    report_stats_();

    return true;
}

void Depacketizer::read_frame_(Frame& frame) {
    if (frame.num_raw_samples() % sample_spec_.num_channels() != 0) {
        roc_panic("depacketizer: unexpected frame size");
    }

    sample_t* buff_ptr = frame.raw_samples();
    sample_t* buff_end = frame.raw_samples() + frame.num_raw_samples();

    FrameInfo info;

    while (buff_ptr < buff_end) {
        buff_ptr = read_samples_(buff_ptr, buff_end, info);
    }

    roc_panic_if(buff_ptr != buff_end);
    set_frame_props_(frame, info);
}

sample_t*
Depacketizer::read_samples_(sample_t* buff_ptr, sample_t* buff_end, FrameInfo& info) {
    update_packet_(info);

    if (packet_) {
        packet::stream_timestamp_t next_timestamp = payload_decoder_.position();

        if (stream_ts_ != next_timestamp) {
            roc_panic_if_not(packet::stream_timestamp_lt(stream_ts_, next_timestamp));

            const size_t mis_samples = sample_spec_.num_channels()
                * (size_t)packet::stream_timestamp_diff(next_timestamp, stream_ts_);

            const size_t max_samples = (size_t)(buff_end - buff_ptr);
            const size_t n_samples = std::min(mis_samples, max_samples);

            buff_ptr = read_missing_samples_(buff_ptr, buff_ptr + n_samples);

            //           next_capture_ts_
            //           next_timestamp
            //                  ↓
            // Packet           |-----------------|
            //           timestamp_
            //               ↓
            // Frame      |----------------|
            info.n_filled_samples += n_samples;
            if (!info.capture_ts && valid_capture_ts_) {
                info.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(info.n_filled_samples);
            }
        }

        if (buff_ptr < buff_end) {
            sample_t* new_buff_ptr = read_packet_samples_(buff_ptr, buff_end);
            const size_t n_samples = size_t(new_buff_ptr - buff_ptr);

            info.n_decoded_samples += n_samples;
            if (n_samples && !info.capture_ts && valid_capture_ts_) {
                info.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(info.n_filled_samples);
            }
            if (valid_capture_ts_) {
                next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
            }

            info.n_filled_samples += n_samples;
            buff_ptr = new_buff_ptr;
        }

        return buff_ptr;
    } else {
        const size_t n_samples = size_t(buff_end - buff_ptr);

        if (!info.capture_ts && valid_capture_ts_) {
            info.capture_ts = next_capture_ts_
                - sample_spec_.samples_overall_2_ns(info.n_filled_samples);
        }
        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
        }

        info.n_filled_samples += n_samples;
        return read_missing_samples_(buff_ptr, buff_end);
    }
}

sample_t* Depacketizer::read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t requested_samples =
        size_t(buff_end - buff_ptr) / sample_spec_.num_channels();

    const size_t decoded_samples = payload_decoder_.read(buff_ptr, requested_samples);

    stream_ts_ += (packet::stream_timestamp_t)decoded_samples;
    packet_samples_ += (packet::stream_timestamp_t)decoded_samples;

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

    stream_ts_ += (packet::stream_timestamp_t)num_samples;

    if (first_packet_) {
        zero_samples_ += (packet::stream_timestamp_t)num_samples;
    } else {
        missing_samples_ += (packet::stream_timestamp_t)num_samples;
    }

    return (buff_ptr + num_samples * sample_spec_.num_channels());
}

void Depacketizer::update_packet_(FrameInfo& info) {
    if (packet_) {
        return;
    }

    packet::stream_timestamp_t pkt_timestamp = 0;
    unsigned n_dropped = 0;

    while ((packet_ = read_packet_())) {
        payload_decoder_.begin(packet_->stream_timestamp(), packet_->payload().data(),
                               packet_->payload().size());

        pkt_timestamp = payload_decoder_.position();

        if (first_packet_) {
            break;
        }

        const packet::stream_timestamp_t pkt_end =
            pkt_timestamp + payload_decoder_.available();

        if (packet::stream_timestamp_lt(stream_ts_, pkt_end)) {
            break;
        }

        roc_log(LogDebug, "depacketizer: dropping late packet: ts=%lu pkt_ts=%lu",
                (unsigned long)stream_ts_, (unsigned long)pkt_timestamp);

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

    next_capture_ts_ = packet_->capture_timestamp();
    if (!valid_capture_ts_ && !!next_capture_ts_) {
        valid_capture_ts_ = true;
    }

    if (first_packet_) {
        roc_log(LogDebug, "depacketizer: got first packet: zero_samples=%lu",
                (unsigned long)zero_samples_);

        stream_ts_ = pkt_timestamp;
        first_packet_ = false;
    }

    // Packet       |-----------------|
    // NextFrame             |----------------|
    if (packet::stream_timestamp_lt(pkt_timestamp, stream_ts_)) {
        const size_t diff_samples =
            (size_t)packet::stream_timestamp_diff(stream_ts_, pkt_timestamp);
        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_per_chan_2_ns(diff_samples);
        }

        if (payload_decoder_.shift(diff_samples) != diff_samples) {
            roc_panic("depacketizer: can't shift packet");
        }
    }
}

packet::PacketPtr Depacketizer::read_packet_() {
    packet::PacketPtr pp;
    const status::StatusCode code = reader_.read(pp);
    if (code != status::StatusOK) {
        if (code != status::StatusNoData) {
            // TODO(gh-302): forward status
            roc_log(LogError, "depacketizer: failed to read packet: status=%s",
                    status::code_to_str(code));
        }

        return NULL;
    }

    return pp;
}

void Depacketizer::set_frame_props_(Frame& frame, const FrameInfo& info) {
    unsigned flags = 0;

    if (info.n_decoded_samples != 0) {
        flags |= Frame::FlagNotBlank;
    }

    if (info.n_decoded_samples < frame.num_raw_samples()) {
        flags |= Frame::FlagNotComplete;
    }

    if (info.n_dropped_packets != 0) {
        flags |= Frame::FlagPacketDrops;
    }

    frame.set_flags(flags);
    frame.set_duration(frame.num_raw_samples() / sample_spec_.num_channels());

    if (info.capture_ts > 0) {
        // do not produce negative cts, which may happen when first packet was in
        // the middle of the frame and has small timestamp close to unix epoch
        frame.set_capture_timestamp(info.capture_ts);
    }
}

void Depacketizer::report_stats_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    const size_t total_samples = missing_samples_ + packet_samples_;
    const double loss_ratio =
        total_samples != 0 ? (double)missing_samples_ / total_samples : 0.;

    roc_log(LogDebug, "depacketizer: ts=%lu loss_ratio=%.5lf", (unsigned long)stream_ts_,
            loss_ratio);
}

} // namespace audio
} // namespace roc
