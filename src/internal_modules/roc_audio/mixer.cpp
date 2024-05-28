/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/mixer.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

Mixer::Mixer(FrameFactory& frame_factory,
             const SampleSpec& sample_spec,
             bool enable_timestamps)
    : frame_factory_(frame_factory)
    , sample_spec_(sample_spec)
    , max_read_(sample_spec.bytes_2_stream_timestamp(frame_factory.byte_buffer_size())
                * sample_spec.num_channels())
    , enable_timestamps_(enable_timestamps)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_valid() || !sample_spec_.is_raw(),
                     "mixer: required valid sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    in_frame_ = frame_factory_.allocate_frame(0);
    if (!in_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Mixer::init_status() const {
    return init_status_;
}

void Mixer::add_input(IFrameReader& reader) {
    roc_panic_if(init_status_ != status::StatusOK);

    frame_readers_.push_back(reader);
}

void Mixer::remove_input(IFrameReader& reader) {
    roc_panic_if(init_status_ != status::StatusOK);

    frame_readers_.remove(reader);
}

status::StatusCode Mixer::read(Frame& out_frame, packet::stream_timestamp_t duration) {
    roc_panic_if(init_status_ != status::StatusOK);

    // If frame doesn't have a buffer, or it's too small for @p duration,
    // allocate and attach buffer.
    if (!frame_factory_.reallocate_frame(
            out_frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    out_frame.set_raw(true);

    unsigned flags = 0;
    core::nanoseconds_t capture_ts = 0;

    const status::StatusCode code =
        mix_all_(out_frame.raw_samples(), out_frame.num_raw_samples(), flags, capture_ts);

    if (code != status::StatusOK) {
        return code;
    }

    out_frame.set_flags(flags);
    out_frame.set_duration(duration);
    out_frame.set_capture_timestamp(capture_ts);

    return status::StatusOK;
}

status::StatusCode Mixer::mix_all_(sample_t* out_data,
                                   size_t out_size,
                                   unsigned& out_flags,
                                   core::nanoseconds_t& out_cts) {
    roc_panic_if(!out_data);
    roc_panic_if(out_size == 0);

    const size_t n_readers = frame_readers_.size();

    core::nanoseconds_t cts_base = 0;
    double cts_sum = 0;
    size_t cts_count = 0;

    memset(out_data, 0, out_size * sizeof(sample_t));

    for (IFrameReader* reader = frame_readers_.front(); reader != NULL;
         reader = frame_readers_.nextof(*reader)) {
        // Mix samples from current reader into output buffer.
        core::nanoseconds_t reader_cts = 0;
        const status::StatusCode code =
            mix_one_(*reader, out_data, out_size, out_flags, reader_cts);

        if (code != status::StatusOK && code != status::StatusDrain) {
            return code;
        }

        if (enable_timestamps_ && reader_cts != 0) {
            // Subtract first non-zero timestamp from all other timestamps.
            // Since timestamp calculation is used only when inputs are synchronous
            // and their timestamps are close, this effectively makes all values
            // small, avoiding overflow and rounding errors when adding them.
            if (cts_base == 0) {
                cts_base = reader_cts;
            }
            cts_sum += double(reader_cts - cts_base);
            cts_count++;
        }
    }

    if (cts_count != 0) {
        // Compute average timestamp.
        // Don't forget to compensate everything that we subtracted above.
        out_cts = core::nanoseconds_t(cts_base * ((double)cts_count / n_readers)
                                      + cts_sum / (double)n_readers);
    }

    return status::StatusOK;
}

status::StatusCode Mixer::mix_one_(IFrameReader& frame_reader,
                                   sample_t* out_data,
                                   size_t out_size,
                                   unsigned& out_flags,
                                   core::nanoseconds_t& out_cts) {
    // In case of partial read, we repeat until we fully fill requested buffer.
    // Pipeline components are allowed to return shorter frames than requested
    // when it simplifies their implementation and expect that Mixer will
    // handle partial reads for them. Mixer, as an entry point to the pipeline,
    // ensures that partial reads never leave pipeline.
    size_t out_pos = 0;
    while (out_pos < out_size) {
        const packet::stream_timestamp_t remained_duration = packet::stream_timestamp_t(
            (out_size - out_pos) / sample_spec_.num_channels());

        const packet::stream_timestamp_t capped_duration =
            sample_spec_.cap_frame_duration(remained_duration,
                                            frame_factory_.byte_buffer_size());

        if (!frame_factory_.reallocate_frame(
                *in_frame_, sample_spec_.stream_timestamp_2_bytes(capped_duration))) {
            return status::StatusNoMem;
        }

        const status::StatusCode code = frame_reader.read(*in_frame_, capped_duration);
        if (code != status::StatusOK && code != status::StatusPart) {
            return code;
        }

        sample_spec_.validate_frame(*in_frame_);

        const size_t in_size = in_frame_->num_raw_samples();
        const sample_t* in_data = in_frame_->raw_samples();

        for (size_t n = 0; n < in_size; n++) {
            // Mix.
            *out_data += *in_data;

            // Saturate on overflow.
            *out_data = std::min(*out_data, Sample_Max);
            *out_data = std::max(*out_data, Sample_Min);

            out_data++;
            in_data++;
        }

        // Accumulate flags from all mixed frames.
        out_flags |= in_frame_->flags();

        // Report CTS of first frame.
        if (out_pos == 0) {
            out_cts = in_frame_->capture_timestamp();
        }

        out_pos += in_size;
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
