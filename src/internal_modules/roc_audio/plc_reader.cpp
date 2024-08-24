/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/plc_reader.h"
#include "roc_audio/format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PlcReader::PlcReader(IFrameReader& frame_reader,
                     FrameFactory& frame_factory,
                     IPlc& plc,
                     const SampleSpec& sample_spec)
    : frame_factory_(frame_factory)
    , frame_reader_(frame_reader)
    , plc_(plc)
    , lookbehind_duration_(plc_.lookbehind_len())
    , lookbehind_byte_size_(sample_spec.stream_timestamp_2_bytes(lookbehind_duration_))
    , lookahead_duration_(plc_.lookahead_len())
    , lookahead_byte_size_(sample_spec.stream_timestamp_2_bytes(lookahead_duration_))
    , ring_frame_pos_(0)
    , ring_frame_size_(0)
    , pending_next_frame_(false)
    , next_frame_pos_(0)
    , got_first_signal_(false)
    , sample_spec_(sample_spec)
    , init_status_(status::NoStatus) {
    if (!sample_spec_.is_complete() || !sample_spec_.is_pcm()) {
        roc_panic("plc reader: required complete sample spec with pcm format: spec=%s",
                  sample_spec_to_str(sample_spec_).c_str());
    }
    if (sample_spec_ != plc_.sample_spec()) {
        roc_panic("plc reader: sample spec mismatch: reader_spec=%s plc_spec=%s",
                  sample_spec_to_str(sample_spec_).c_str(),
                  sample_spec_to_str(plc_.sample_spec()).c_str());
    }

    roc_log(LogDebug,
            "plc reader: initializing:"
            " lookbehind=%lu(%.3fms) lookahead=%lu(%.3fms) sample_spec=%s",
            (unsigned long)lookbehind_duration_,
            (double)sample_spec_.stream_timestamp_2_ms(lookbehind_duration_),
            (unsigned long)lookahead_duration_,
            (double)sample_spec_.stream_timestamp_2_ms(lookahead_duration_),
            sample_spec_to_str(sample_spec_).c_str());

    if (lookbehind_byte_size_ > frame_factory_.byte_buffer_size()
        || lookahead_byte_size_ > frame_factory_.byte_buffer_size()) {
        init_status_ = status::StatusNoMem;
        return;
    }

    if (lookbehind_duration_ > 0) {
        prev_frame_ = frame_factory_.allocate_frame(lookbehind_byte_size_);
        if (!prev_frame_) {
            init_status_ = status::StatusNoMem;
            return;
        }

        ring_frame_ = frame_factory_.allocate_frame(lookbehind_byte_size_);
        if (!ring_frame_) {
            init_status_ = status::StatusNoMem;
            return;
        }
    }

    if (lookahead_duration_ > 0) {
        next_frame_ = frame_factory_.allocate_frame(lookahead_byte_size_);
        if (!next_frame_) {
            init_status_ = status::StatusNoMem;
            return;
        }

        temp_frame_ = frame_factory_.allocate_frame(lookahead_byte_size_);
        if (!temp_frame_) {
            init_status_ = status::StatusNoMem;
            return;
        }
    }

    init_status_ = status::StatusOK;
}

status::StatusCode PlcReader::init_status() const {
    return init_status_;
}

status::StatusCode PlcReader::read(Frame& frame,
                                   packet::stream_timestamp_t requested_duration,
                                   FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    const packet::stream_timestamp_t capped_duration = sample_spec_.cap_frame_duration(
        requested_duration, frame_factory_.byte_buffer_size());

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(capped_duration))) {
        return status::StatusNoMem;
    }

    status::StatusCode code = status::NoStatus;

    if (pending_next_frame_) {
        // We did a successful read-ahead recently. We should return samples from
        // saved frame until it becomes empty.
        code = read_from_memory_(frame, capped_duration);
    } else {
        // Normal read from underlying reader.
        code = read_from_reader_(frame, capped_duration, mode);
    }

    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    if (lookbehind_duration_ > 0) {
        // Update ring buffer with history.
        append_history_(frame);
    }

    const packet::stream_timestamp_t resulted_duration = frame.duration();

    return resulted_duration == requested_duration ? status::StatusOK
                                                   : status::StatusPart;
}

status::StatusCode
PlcReader::read_from_memory_(Frame& frame,
                             packet::stream_timestamp_t requested_duration) {
    const packet::stream_timestamp_t avail_duration =
        std::min(requested_duration,
                 sample_spec_.bytes_2_stream_timestamp(next_frame_->num_bytes()
                                                       - next_frame_pos_));

    const size_t avail_bytes = sample_spec_.stream_timestamp_2_bytes(avail_duration);

    roc_panic_if(avail_duration == 0 || avail_bytes == 0);
    roc_panic_if(next_frame_pos_ + avail_bytes > next_frame_->num_bytes());

    frame.set_flags(next_frame_->flags());
    frame.set_raw(sample_spec_.is_raw());
    frame.set_duration(avail_duration);
    frame.set_num_bytes(avail_bytes);

    if (next_frame_->capture_timestamp() != 0) {
        frame.set_capture_timestamp(next_frame_->capture_timestamp()
                                    + sample_spec_.bytes_2_ns(next_frame_pos_));
    }

    memcpy(frame.bytes(), next_frame_->bytes() + next_frame_pos_, avail_bytes);

    // Give frame to PLC for research purposes.
    plc_.process_history(frame);

    next_frame_pos_ += avail_bytes;
    if (next_frame_pos_ == next_frame_->num_bytes()) {
        // We've fully read saved frame, now we can switch to normal reads.
        pending_next_frame_ = false;
        next_frame_pos_ = 0;
    }

    return avail_duration == requested_duration ? status::StatusOK : status::StatusPart;
}

status::StatusCode PlcReader::read_from_reader_(
    Frame& frame, packet::stream_timestamp_t requested_duration, FrameReadMode mode) {
    const status::StatusCode code = frame_reader_.read(frame, requested_duration, mode);
    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    // We rely on the fact that depacketizer returns frames that are either
    // entirely signal or entirely gap.
    roc_panic_if_msg(
        frame.has_flags(Frame::HasSignal) == frame.has_flags(Frame::HasGaps),
        "plc reader: unexpected frame flags: must have either signal or gaps");

    sample_spec_.validate_frame(frame);

    if (!frame.has_flags(Frame::HasGaps)) {
        // Good frame, give it to PLC for research purposes.
        plc_.process_history(frame);
        got_first_signal_ = true;
    } else if (got_first_signal_) {
        // Gap frame (with zeros), ask PLC to fill it with interpolated data.
        Frame* prev_frame_ptr = NULL;
        Frame* next_frame_ptr = NULL;

        if (lookahead_duration_ != 0) {
            const status::StatusCode code = try_read_next_frame_();
            if (code != status::StatusOK && code != status::StatusPart
                && code != status::StatusDrain) {
                return code;
            }
            if (pending_next_frame_) {
                // Next frame may not be available if packets haven't arrived yet.
                next_frame_ptr = next_frame_.get();
            }
        }

        if (lookbehind_duration_ != 0) {
            // Prev frame is always available, we build it from history ring buffer.
            const status::StatusCode code = build_prev_frame_();
            if (code != status::StatusOK) {
                return code;
            }
            prev_frame_ptr = prev_frame_.get();
        }

        plc_.process_loss(frame, prev_frame_ptr, next_frame_ptr);
    }

    return frame.duration() == requested_duration ? status::StatusOK : status::StatusPart;
}

// Perform a soft read to get next frame, but only while there are no gaps.
status::StatusCode PlcReader::try_read_next_frame_() {
    roc_panic_if(lookahead_duration_ == 0);
    roc_panic_if(pending_next_frame_);

    if (!frame_factory_.reallocate_frame(*next_frame_, lookahead_byte_size_)) {
        return status::StatusNoMem;
    }

    packet::stream_timestamp_t frame_duration = 0;
    size_t frame_size = 0;
    unsigned frame_flags = 0;
    core::nanoseconds_t frame_cts = 0;

    // If soft read returns StatusPart, repeat read and concatenate results.
    // Partial reads may be caused by buffering limitations, however we want
    // to gather the full look-ahead length if it's possible.
    while (frame_duration < lookahead_duration_) {
        if (!frame_factory_.reallocate_frame(*temp_frame_, lookahead_byte_size_)) {
            return status::StatusNoMem;
        }

        const status::StatusCode code = frame_reader_.read(
            *temp_frame_, lookahead_duration_ - frame_duration, ModeSoft);

        if (code != status::StatusOK && code != status::StatusPart
            && code != status::StatusDrain) {
            // Soft read reports failure.
            return code;
        }

        if (code == status::StatusDrain) {
            // Soft read reports that the next packet haven't arrived yet.
            break;
        }

        roc_panic_if_msg(
            !temp_frame_->has_flags(Frame::HasSignal)
                || temp_frame_->has_flags(Frame::HasGaps),
            "plc reader: unexpected frame flags from soft read: must have signal");

        sample_spec_.validate_frame(*temp_frame_);

        if (code == status::StatusOK && frame_size == 0) {
            // Happy path: we've read the whole frame, and there is no need to do
            // concatenation, we can just use temp frame as next frame. Use swap
            // to keep the second frame cached, to avoid allocation later.
            std::swap(next_frame_, temp_frame_);

            pending_next_frame_ = true;
            return status::StatusOK;
        }

        memcpy(next_frame_->bytes() + frame_size, temp_frame_->bytes(),
               temp_frame_->num_bytes());

        if (frame_size == 0) {
            frame_cts = temp_frame_->capture_timestamp();
        }
        frame_duration += temp_frame_->duration();
        frame_size += temp_frame_->num_bytes();
        frame_flags |= temp_frame_->flags();
    }

    if (frame_duration == 0) {
        return status::StatusDrain;
    }

    next_frame_->set_flags(frame_flags);
    next_frame_->set_raw(sample_spec_.is_raw());
    next_frame_->set_duration(frame_duration);
    next_frame_->set_num_bytes(frame_size);
    next_frame_->set_capture_timestamp(frame_cts);

    pending_next_frame_ = true;
    return status::StatusOK;
}

// Copy samples from history ring buffer to a continuous frame.
status::StatusCode PlcReader::build_prev_frame_() {
    roc_panic_if(lookbehind_duration_ == 0);

    roc_panic_if(ring_frame_pos_ > lookbehind_byte_size_);
    roc_panic_if(ring_frame_size_ == 0 || ring_frame_size_ > lookbehind_byte_size_);

    const size_t dst_size = ring_frame_size_;
    uint8_t* dst_data = prev_frame_->bytes();

    if (!frame_factory_.reallocate_frame(*prev_frame_, dst_size)) {
        return status::StatusNoMem;
    }

    const size_t lo_size = std::min(lookbehind_byte_size_ - ring_frame_pos_, dst_size);
    const uint8_t* lo_data = ring_frame_->bytes() + ring_frame_pos_;

    memcpy(dst_data, lo_data, lo_size);

    if (lo_size < dst_size) {
        const size_t hi_size = dst_size - lo_size;
        uint8_t* hi_data = ring_frame_->bytes();

        memcpy(dst_data + lo_size, hi_data, hi_size);
    }

    prev_frame_->set_duration(sample_spec_.bytes_2_stream_timestamp(dst_size));
    prev_frame_->set_raw(sample_spec_.is_raw());

    return status::StatusOK;
}

// Add frame to history ring buffer.
void PlcReader::append_history_(Frame& frame) {
    roc_panic_if(lookbehind_duration_ == 0);

    roc_panic_if(ring_frame_pos_ > lookbehind_byte_size_);
    roc_panic_if(ring_frame_size_ > lookbehind_byte_size_);

    const size_t src_size = std::min(frame.num_bytes(), lookbehind_byte_size_);
    const uint8_t* src_data = frame.bytes() + frame.num_bytes() - src_size;

    const size_t lo_pos = (ring_frame_pos_ + ring_frame_size_) % lookbehind_byte_size_;
    const size_t lo_size = std::min(lookbehind_byte_size_ - lo_pos, src_size);
    uint8_t* lo_data = ring_frame_->bytes() + lo_pos;

    memcpy(lo_data, src_data, lo_size);

    if (lo_size < src_size) {
        const size_t hi_size = src_size - lo_size;
        uint8_t* hi_data = ring_frame_->bytes();

        memcpy(hi_data, src_data + lo_size, hi_size);
    }

    ring_frame_size_ += src_size;
    if (ring_frame_size_ > lookbehind_byte_size_) {
        ring_frame_pos_ = (ring_frame_pos_ + (ring_frame_size_ - lookbehind_byte_size_))
            % lookbehind_byte_size_;
        ring_frame_size_ = lookbehind_byte_size_;
    }
}

} // namespace audio
} // namespace roc
