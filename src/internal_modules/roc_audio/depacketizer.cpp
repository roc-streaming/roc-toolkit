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

const core::nanoseconds_t LogInterval = 30 * core::Second;

} // namespace

Depacketizer::Depacketizer(packet::IReader& packet_reader,
                           IFrameDecoder& payload_decoder,
                           FrameFactory& frame_factory,
                           const SampleSpec& sample_spec,
                           dbgio::CsvDumper* dumper)
    : frame_factory_(frame_factory)
    , packet_reader_(packet_reader)
    , payload_decoder_(payload_decoder)
    , sample_spec_(sample_spec)
    , stream_ts_(0)
    , next_capture_ts_(0)
    , valid_capture_ts_(false)
    , decoded_samples_(0)
    , missing_samples_(0)
    , late_samples_(0)
    , recovered_samples_(0)
    , is_started_(false)
    , rate_limiter_(LogInterval)
    , dumper_(dumper)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_complete() || !sample_spec_.is_raw(),
                     "depacketizer: required complete sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    roc_log(LogDebug, "depacketizer: initializing: sample_rate=%lu n_channels=%lu",
            (unsigned long)sample_spec_.sample_rate(),
            (unsigned long)sample_spec_.num_channels());

    init_status_ = status::StatusOK;
}

status::StatusCode Depacketizer::init_status() const {
    return init_status_;
}

bool Depacketizer::is_started() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return is_started_;
}

const DepacketizerMetrics& Depacketizer::metrics() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return metrics_;
}

packet::stream_timestamp_t Depacketizer::next_timestamp() const {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!is_started_) {
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

    FrameStats frame_stats;

    sample_t* buff_ptr = frame.raw_samples();
    sample_t* buff_end = frame.raw_samples() + frame.num_raw_samples();

    while (buff_ptr < buff_end) {
        const size_t requested_samples = size_t(buff_end - buff_ptr);
        const status::StatusCode code =
            update_packet_(requested_samples, mode, frame_stats);

        if (code == status::StatusDrain && mode == ModeSoft) {
            break; // In soft read mode, stop on packet loss.
        }
        if (code != status::StatusOK && code != status::StatusDrain) {
            return code;
        }

        sample_t* next_buff_ptr = read_samples_(buff_ptr, buff_end, mode, frame_stats);
        if (!next_buff_ptr) { // Partial or drained read.
            break;
        }

        buff_ptr = next_buff_ptr;
    }

    const size_t frame_samples = size_t(buff_ptr - frame.raw_samples());

    roc_panic_if_not(buff_ptr >= frame.raw_samples() && buff_ptr <= buff_end);
    roc_panic_if_not(frame_samples == frame_stats.n_written_samples);

    if (frame_samples == 0) {
        roc_panic_if(mode != ModeSoft);
        return status::StatusDrain;
    }

    commit_frame_(frame, frame_samples, frame_stats);

    periodic_report_();
    if (dumper_) {
        dump_();
    }

    return frame.duration() == requested_duration ? status::StatusOK : status::StatusPart;
}

sample_t* Depacketizer::read_samples_(sample_t* buff_ptr,
                                      sample_t* buff_end,
                                      FrameReadMode mode,
                                      FrameStats& stats) {
    if (packet_) {
        packet::stream_timestamp_t next_decoder_ts = payload_decoder_.position();

        if (packet::stream_timestamp_lt(stream_ts_, next_decoder_ts)) {
            // If there is a gap between stream timestamp and packet decoder timestamp,
            // fill the gap with zeros.
            //
            //              next_decoder_ts, next_capture_ts_
            //                     ↓
            // Packet             |■■■■■■■■■••••••••|
            //             stream_ts_
            //                ↓
            // Frame      |•••□□□□□■■■■■■■■■|
            //                 gap
            if (mode == ModeSoft || stats.n_decoded_samples != 0) {
                // In soft mode, stop reading on gap.
                // Also, in any mode, don't mix signal and gaps in one frame.
                roc_panic_if_not(mode == ModeSoft || stats.n_written_samples > 0);
                return NULL;
            }

            const size_t max_samples = (size_t)(buff_end - buff_ptr);
            const size_t mis_samples =
                (size_t)packet::stream_timestamp_diff(next_decoder_ts, stream_ts_)
                * sample_spec_.num_channels();

            const size_t n_samples = std::min(mis_samples, max_samples);

            sample_t* next_buff_ptr =
                read_missing_samples_(buff_ptr, buff_ptr + n_samples);

            stats.n_written_samples += n_samples;
            stats.n_missing_samples += n_samples;

            if (!stats.capture_ts && valid_capture_ts_) {
                stats.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(stats.n_written_samples);
            }

            return next_buff_ptr;
        } else {
            // If stream timestamp is aligned with packet decoder timestamp, decode
            // samples from packet into frame.
            //
            //           next_decoder_ts, next_capture_ts_
            //                  ↓
            // Packet          |■■■■■■■■■■■••••••|
            //               stream_ts_
            //                  ↓
            // Frame      |•••••■■■■■■■■■■■|
            roc_panic_if_msg(stream_ts_ != next_decoder_ts,
                             // Can't happen because of the logic in start_packet_().
                             "depacketizer: inconsistent stream and decoder timestamps");

            if (stats.n_missing_samples != 0) {
                // Don't mix signal and losses in one frame.
                roc_panic_if_not(mode == ModeSoft || stats.n_written_samples > 0);
                return NULL;
            }

            sample_t* next_buff_ptr = read_decoded_samples_(buff_ptr, buff_end);

            const size_t n_samples = size_t(next_buff_ptr - buff_ptr);

            if (n_samples && !stats.capture_ts && valid_capture_ts_) {
                stats.capture_ts = next_capture_ts_
                    - sample_spec_.samples_overall_2_ns(stats.n_written_samples);
            }
            if (valid_capture_ts_) {
                next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
            }

            stats.n_written_samples += n_samples;
            stats.n_decoded_samples += n_samples;

            return next_buff_ptr;
        }
    } else {
        // If there is no packet, fill requested buffer with zeros.
        //
        //                stream_ts_
        //                   ↓
        // Frame      |••••••□□□□□□□□□□□|
        //                       gap
        if (mode == ModeSoft || stats.n_decoded_samples != 0) {
            // In soft mode, stop reading on loss.
            // Also, in any mode, don't mix signal and losses in one frame.
            roc_panic_if_not(mode == ModeSoft || stats.n_written_samples > 0);
            return NULL;
        }

        const size_t n_samples = size_t(buff_end - buff_ptr);

        if (!stats.capture_ts && valid_capture_ts_) {
            stats.capture_ts = next_capture_ts_
                - sample_spec_.samples_overall_2_ns(stats.n_written_samples);
        }
        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_overall_2_ns(n_samples);
        }

        stats.n_written_samples += n_samples;
        stats.n_missing_samples += n_samples;

        sample_t* next_buff_ptr = read_missing_samples_(buff_ptr, buff_end);

        return next_buff_ptr;
    }
}

sample_t* Depacketizer::read_decoded_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t requested_samples =
        size_t(buff_end - buff_ptr) / sample_spec_.num_channels();

    const size_t decoded_samples =
        payload_decoder_.read_samples(buff_ptr, requested_samples);

    stream_ts_ += (packet::stream_timestamp_t)decoded_samples;
    decoded_samples_ += decoded_samples;
    metrics_.decoded_samples += decoded_samples;

    if (packet_->has_flags(packet::Packet::FlagRestored)) {
        recovered_samples_ += decoded_samples;
        metrics_.recovered_samples += decoded_samples;
    }

    if (decoded_samples < requested_samples) {
        payload_decoder_.end_frame();
        packet_ = NULL;
    }

    return (buff_ptr + decoded_samples * sample_spec_.num_channels());
}

sample_t* Depacketizer::read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end) {
    const size_t missing_samples =
        (size_t)(buff_end - buff_ptr) / sample_spec_.num_channels();

    memset(buff_ptr, 0, missing_samples * sample_spec_.num_channels() * sizeof(sample_t));

    stream_ts_ += (packet::stream_timestamp_t)missing_samples;
    missing_samples_ += missing_samples;
    if (is_started_) {
        metrics_.missing_samples += missing_samples;
    }

    return (buff_ptr + missing_samples * sample_spec_.num_channels());
}

status::StatusCode Depacketizer::update_packet_(size_t requested_samples,
                                                FrameReadMode mode,
                                                FrameStats& stats) {
    if (packet_) { // Already have packet.
        return status::StatusOK;
    }

    for (;;) {
        status::StatusCode code = fetch_packet_(requested_samples, mode);
        if (code == status::StatusDrain) {
            break; // No viable packets.
        }
        if (code != status::StatusOK) {
            return code;
        }

        if ((code = start_packet_()) != status::StatusOK) {
            return code;
        }
        if (packet_) {
            break;
        }

        // Packet dropped, try another one.
        stats.n_dropped_packets++;
    }

    return status::StatusOK;
}

status::StatusCode Depacketizer::fetch_packet_(size_t requested_samples,
                                               FrameReadMode mode) {
    roc_panic_if(packet_);

    // Disable soft reads until we initialize stream timestamps.
    if (!is_started_ && mode == ModeSoft) {
        return status::StatusDrain;
    }

    if (is_started_) {
        // Region which we want to decode.
        const packet::stream_timestamp_t frame_begin = stream_ts_;
        const packet::stream_timestamp_t frame_end = stream_ts_
            + packet::stream_timestamp_t(requested_samples / sample_spec_.num_channels());

        // Get packet without removing it from queue.
        packet::PacketPtr pkt;
        const status::StatusCode code = packet_reader_.read(pkt, packet::ModePeek);
        if (code != status::StatusOK && code != status::StatusDrain) {
            roc_panic_if(code == status::StatusPart);
            roc_log(LogError, "depacketizer: failed to read packet: mode=peek status=%s",
                    status::code_to_str(code));
            return code;
        }

        // In soft read mode, if there is a gap between current timestamp and next
        // available packet (or there is no packet), stop reading.
        if (mode == ModeSoft
            && (!pkt
                || packet::stream_timestamp_lt(frame_begin, pkt->stream_timestamp()))) {
            return status::StatusDrain;
        }

        // If next available packet is outside of the decode range, there is no need
        // to fetch it now. We should give a chance for more packets to arrive.
        if (pkt && packet::stream_timestamp_le(frame_end, pkt->stream_timestamp())) {
            roc_log(LogTrace,
                    "depacketizer: keeping packet in queue:"
                    " stream_ts=%lu end_ts=%lu pkt_ts=%lu",
                    (unsigned long)stream_ts_, (unsigned long)frame_end,
                    (unsigned long)pkt->stream_timestamp());
            return status::StatusDrain;
        }
    }

    // The packet is viable, fetch it.
    packet::PacketPtr pkt;
    const status::StatusCode code = packet_reader_.read(pkt, packet::ModeFetch);
    if (code != status::StatusOK) {
        if (code != status::StatusDrain) {
            roc_panic_if(code == status::StatusPart);
            roc_log(LogError, "depacketizer: failed to read packet: mode=fetch status=%s",
                    status::code_to_str(code));
        }
        return code;
    }

    roc_panic_if(!pkt);
    packet_ = pkt;

    return code;
}

status::StatusCode Depacketizer::start_packet_() {
    roc_panic_if(!packet_);

    payload_decoder_.begin_frame(packet_->stream_timestamp(), packet_->payload().data(),
                                 packet_->payload().size());

    const packet::stream_timestamp_t pkt_begin = payload_decoder_.position();
    const packet::stream_timestamp_t pkt_end = pkt_begin + payload_decoder_.available();

    // If packet ends before current stream position, drop the whole packet.
    //
    //                         stream_ts_
    //                             ↓
    //  Frame                |■■■■■•••••••••••|
    //                          pkt_end
    //                             ↓
    //  Packet      |□□□□□□□□□□□□□□|
    if (is_started_ && packet::stream_timestamp_le(pkt_end, stream_ts_)) {
        roc_log(LogTrace, "depacketizer: dropping late packet: stream_ts=%lu pkt_ts=%lu",
                (unsigned long)stream_ts_, (unsigned long)pkt_begin);

        late_samples_ += pkt_end - pkt_begin;
        metrics_.late_samples += pkt_end - pkt_begin;
        metrics_.late_packets++;

        payload_decoder_.end_frame();
        packet_ = NULL;

        return status::StatusOK;
    }

    next_capture_ts_ = packet_->capture_timestamp();
    if (!valid_capture_ts_ && !!next_capture_ts_) {
        valid_capture_ts_ = true;
    }

    if (!is_started_) {
        roc_log(LogDebug,
                "depacketizer: got first packet: start_ts=%lu start_latency=%lu",
                (unsigned long)pkt_begin, (unsigned long)missing_samples_);

        stream_ts_ = pkt_begin;
        missing_samples_ = 0;
        is_started_ = true;
    }

    // If packet begins before current stream position, drop samples from
    // the beginning of the packet.
    //
    //                    stream_ts_
    //                        ↓
    // Frame            |•••••■■■■■•••••••|
    //           pkt_begin
    //               ↓
    //  Packet      |□□□□□□□□□■■■■■|
    if (packet::stream_timestamp_lt(pkt_begin, stream_ts_)) {
        const size_t diff_samples =
            (size_t)packet::stream_timestamp_diff(stream_ts_, pkt_begin);

        roc_log(LogTrace,
                "depacketizer: dropping samples: stream_ts=%lu pkt_ts=%lu diff=%lu",
                (unsigned long)stream_ts_, (unsigned long)pkt_begin,
                (unsigned long)diff_samples);

        late_samples_ += diff_samples;
        metrics_.late_samples += diff_samples;
        metrics_.late_packets++;

        if (valid_capture_ts_) {
            next_capture_ts_ += sample_spec_.samples_per_chan_2_ns(diff_samples);
        }

        if (payload_decoder_.drop_samples(diff_samples) != diff_samples) {
            roc_panic("depacketizer: can't drop samples from decoder");
        }
    }

    metrics_.decoded_packets++;
    if (packet_->has_flags(packet::Packet::FlagRestored)) {
        metrics_.recovered_packets++;
    }

    return status::StatusOK;
}

void Depacketizer::commit_frame_(Frame& frame,
                                 size_t frame_samples,
                                 const FrameStats& frame_stats) {
    roc_panic_if_msg(frame_stats.n_written_samples
                         != frame_stats.n_decoded_samples + frame_stats.n_missing_samples,
                     "depacketizer: incorrect sample counters");

    roc_panic_if_msg(frame_stats.n_decoded_samples != 0
                         && frame_stats.n_missing_samples != 0,
                     "depacketizer: incorrect sample counters");

    roc_log(LogTrace,
            "depacketizer: returning frame:"
            " stream_ts=%lu n_decoded=%lu n_missing=%lu n_dropped=%lu",
            (unsigned long)stream_ts_
                - (frame_stats.n_written_samples / sample_spec_.num_channels()),
            (unsigned long)frame_stats.n_decoded_samples / sample_spec_.num_channels(),
            (unsigned long)frame_stats.n_missing_samples / sample_spec_.num_channels(),
            (unsigned long)frame_stats.n_dropped_packets);

    unsigned flags = 0;
    if (frame_stats.n_decoded_samples != 0) {
        flags |= Frame::HasSignal;
    }
    if (frame_stats.n_missing_samples != 0) {
        flags |= Frame::HasGaps;
    }
    if (frame_stats.n_dropped_packets != 0) {
        flags |= Frame::HasDrops;
    }

    frame.set_flags(flags);
    frame.set_num_raw_samples(frame_samples);
    frame.set_duration(
        packet::stream_timestamp_t(frame_samples / sample_spec_.num_channels()));

    if (frame_stats.capture_ts > 0) {
        // Do not produce negative cts, which may happen when first packet was in
        // the middle of the frame and has small timestamp close to unix epoch.
        frame.set_capture_timestamp(frame_stats.capture_ts);
    }
}

void Depacketizer::periodic_report_() {
    if (!rate_limiter_.allow() || !is_started_) {
        return;
    }

    const size_t total_samples = decoded_samples_ + missing_samples_;

    roc_log(LogDebug,
            "depacketizer:"
            " period=%.2fms missing=%.2fms(%.3f%%)"
            " late=%.2fms(%.3f%%) recovered=%.2fms(%.3f%%)",
            sample_spec_.stream_timestamp_2_ms(total_samples),
            sample_spec_.stream_timestamp_2_ms(missing_samples_),
            (double)missing_samples_ / total_samples * 100,
            sample_spec_.stream_timestamp_2_ms(late_samples_),
            (double)late_samples_ / total_samples * 100,
            sample_spec_.stream_timestamp_2_ms(recovered_samples_),
            (double)recovered_samples_ / total_samples * 100);

    decoded_samples_ = 0;
    missing_samples_ = 0;
    late_samples_ = 0;
    recovered_samples_ = 0;
}

void Depacketizer::dump_() {
    dbgio::CsvEntry e;
    e.type = 'd';
    e.n_fields = 4;
    e.fields[0] = core::timestamp(core::ClockUnix);
    e.fields[1] = metrics_.missing_samples;
    e.fields[2] = metrics_.late_samples;
    e.fields[3] = metrics_.recovered_samples;

    dumper_->write(e);
}

} // namespace audio
} // namespace roc
