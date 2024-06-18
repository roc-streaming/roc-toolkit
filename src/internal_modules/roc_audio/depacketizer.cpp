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

Depacketizer::Depacketizer(packet::IReader& packet_reader,
                           IFrameDecoder& payload_decoder,
                           FrameFactory& frame_factory,
                           const SampleSpec& sample_spec,
                           bool beep)
    : frame_factory_(frame_factory)
    , packet_reader_(packet_reader)
    , payload_decoder_(payload_decoder)
    , sample_spec_(sample_spec)
    , stream_ts_(0)
    , next_capture_ts_(0)
    , valid_capture_ts_(false)
    , padding_samples_(0)
    , missing_samples_(0)
    , decoded_samples_(0)
    , fetched_packets_(0)
    , dropped_packets_(0)
    , rate_limiter_(LogInterval)
    , beep_(beep)
    , first_packet_(true)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_valid() || !sample_spec_.is_raw(),
                     "depacketizer: required valid sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    roc_log(LogDebug, "depacketizer: initializing: n_channels=%lu",
            (unsigned long)sample_spec_.num_channels());

    init_status_ = status::StatusOK;
}

status::StatusCode Depacketizer::init_status() const {
    return init_status_;
}

bool Depacketizer::is_started() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return !first_packet_;
}

packet::stream_timestamp_t Depacketizer::next_timestamp() const {
    roc_panic_if(init_status_ != status::StatusOK);

    if (first_packet_) {
        return 0;
    }
    return stream_ts_;
}

status::StatusCode Depacketizer::read(Frame& frame,
                                      packet::stream_timestamp_t requested_duration,
                                      FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    const packet::stream_timestamp_t capped_duration = sample_spec_.cap_frame_duration(
        requested_duration, frame_factory_.byte_buffer_size());

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(capped_duration))) {
        return status::StatusNoMem;
    }

    frame.set_raw(true);

    sample_t* buff_ptr = frame.raw_samples();
    sample_t* buff_end = frame.raw_samples() + frame.num_raw_samples();

    FrameStats frame_stats;

    while (buff_ptr < buff_end) {
        const status::StatusCode code = update_packet_(frame_stats);
        if (code != status::StatusOK && code != status::StatusDrain) {
            return code;
        }

        buff_ptr = read_samples_(buff_ptr, buff_end, frame_stats);
    }
    roc_panic_if(buff_ptr != buff_end);

    commit_frame_(frame, frame_stats);
    report_stats_();

    return capped_duration == requested_duration ? status::StatusOK : status::StatusPart;
}

sample_t*
Depacketizer::read_samples_(sample_t* buff_ptr, sample_t* buff_end, FrameStats& stats) {
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
            stats.n_filled_samples += n_samples;
            if (!stats.capture_ts && valid_capture_ts_) {
                stats.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(stats.n_filled_samples);
            }
        }

        if (buff_ptr < buff_end) {
            sample_t* new_buff_ptr = read_packet_samples_(buff_ptr, buff_end);
            const size_t n_samples = size_t(new_buff_ptr - buff_ptr);

            stats.n_decoded_samples += n_samples;
            if (n_samples && !stats.capture_ts && valid_capture_ts_) {
                stats.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(stats.n_filled_samples);
            }
            if (valid_capture_ts_) {
                next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
            }

            stats.n_filled_samples += n_samples;
            buff_ptr = new_buff_ptr;
        }

        return buff_ptr;
    } else {
        const size_t n_samples = size_t(buff_end - buff_ptr);

        if (!stats.capture_ts && valid_capture_ts_) {
            stats.capture_ts = next_capture_ts_
                - sample_spec_.samples_overall_2_ns(stats.n_filled_samples);
        }
        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
        }

        stats.n_filled_samples += n_samples;
        return read_missing_samples_(buff_ptr, buff_end);
    }
}

sample_t* Depacketizer::read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t requested_samples =
        size_t(buff_end - buff_ptr) / sample_spec_.num_channels();

    const size_t decoded_samples = payload_decoder_.read(buff_ptr, requested_samples);

    stream_ts_ += (packet::stream_timestamp_t)decoded_samples;
    decoded_samples_ += (packet::stream_timestamp_t)decoded_samples;

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
        padding_samples_ += (packet::stream_timestamp_t)num_samples;
    } else {
        missing_samples_ += (packet::stream_timestamp_t)num_samples;
    }

    return (buff_ptr + num_samples * sample_spec_.num_channels());
}

status::StatusCode Depacketizer::update_packet_(FrameStats& frame_stats) {
    if (packet_) {
        // Already have packet.
        return status::StatusOK;
    }

    for (;;) {
        const status::StatusCode code = fetch_packet_();
        if (code != status::StatusOK) {
            return code;
        }

        if (start_packet_()) {
            break;
        }

        // Try next packet.
        frame_stats.n_dropped_packets++;
    }

    roc_panic_if(!packet_);

    return status::StatusOK;
}

status::StatusCode Depacketizer::fetch_packet_() {
    packet::PacketPtr pp;
    const status::StatusCode code = packet_reader_.read(pp);

    if (code != status::StatusOK) {
        if (code != status::StatusDrain) {
            roc_log(LogError, "depacketizer: failed to read packet: status=%s",
                    status::code_to_str(code));
        }
        return code;
    }

    packet_ = pp;
    fetched_packets_++;

    return status::StatusOK;
}

bool Depacketizer::start_packet_() {
    roc_panic_if(!packet_);

    payload_decoder_.begin(packet_->stream_timestamp(), packet_->payload().data(),
                           packet_->payload().size());

    const packet::stream_timestamp_t pkt_begin = payload_decoder_.position();
    const packet::stream_timestamp_t pkt_end = pkt_begin + payload_decoder_.available();

    if (!first_packet_ && !packet::stream_timestamp_lt(stream_ts_, pkt_end)) {
        roc_log(LogTrace, "depacketizer: dropping late packet: stream_ts=%lu pkt_ts=%lu",
                (unsigned long)stream_ts_, (unsigned long)pkt_begin);

        dropped_packets_++;

        payload_decoder_.end();
        packet_ = NULL;

        return false;
    }

    next_capture_ts_ = packet_->capture_timestamp();
    if (!valid_capture_ts_ && !!next_capture_ts_) {
        valid_capture_ts_ = true;
    }

    if (first_packet_) {
        roc_log(LogDebug, "depacketizer: got first packet: padding_samples=%lu",
                (unsigned long)padding_samples_);

        stream_ts_ = pkt_begin;
        first_packet_ = false;
    }

    // Packet       |-----------------|
    // NextFrame             |----------------|
    if (packet::stream_timestamp_lt(pkt_begin, stream_ts_)) {
        const size_t diff_samples =
            (size_t)packet::stream_timestamp_diff(stream_ts_, pkt_begin);
        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_per_chan_2_ns(diff_samples);
        }

        if (payload_decoder_.shift(diff_samples) != diff_samples) {
            roc_panic("depacketizer: can't shift packet");
        }
    }

    return true;
}

void Depacketizer::commit_frame_(Frame& frame, const FrameStats& frame_stats) {
    unsigned flags = 0;

    if (frame_stats.n_decoded_samples != 0) {
        flags |= Frame::HasSignal;
    }

    if (frame_stats.n_decoded_samples < frame.num_raw_samples()) {
        flags |= Frame::HasGaps;
    }

    if (frame_stats.n_dropped_packets != 0) {
        flags |= Frame::HasDrops;
    }

    frame.set_flags(flags);
    frame.set_duration(frame.num_raw_samples() / sample_spec_.num_channels());

    if (frame_stats.capture_ts > 0) {
        // Do not produce negative cts, which may happen when first packet was in
        // the middle of the frame and has small timestamp close to unix epoch.
        frame.set_capture_timestamp(frame_stats.capture_ts);
    }
}

void Depacketizer::report_stats_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    const double loss_ratio = (missing_samples_ + decoded_samples_) != 0
        ? (double)missing_samples_ / (missing_samples_ + decoded_samples_)
        : 0.;

    const double drop_ratio =
        fetched_packets_ != 0 ? (double)dropped_packets_ / fetched_packets_ : 0.;

    roc_log(LogDebug,
            "depacketizer:"
            " fetched_pkts=%lu dropped_pkts=%lu loss_ratio=%.5lf late_ratio=%.5lf",
            (unsigned long)fetched_packets_, (unsigned long)dropped_packets_, loss_ratio,
            drop_ratio);

    missing_samples_ = decoded_samples_ = 0;
    fetched_packets_ = dropped_packets_ = 0;
}

} // namespace audio
} // namespace roc
