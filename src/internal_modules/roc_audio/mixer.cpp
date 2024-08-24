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
#include "roc_status/code_to_str.h"

namespace roc {
namespace audio {

Mixer::Mixer(const SampleSpec& sample_spec,
             bool enable_timestamps,
             FrameFactory& frame_factory,
             core::IArena& arena)
    : frame_factory_(frame_factory)
    , inputs_(arena)
    , sample_spec_(sample_spec)
    , enable_timestamps_(enable_timestamps)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_complete() || !sample_spec_.is_raw(),
                     "mixer: required complete sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    in_frame_ = frame_factory_.allocate_frame(0);
    if (!in_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    mix_buffer_ = frame_factory_.new_raw_buffer();
    if (!mix_buffer_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    memset(mix_buffer_.data(), 0, mix_buffer_.size() * sizeof(sample_t));

    init_status_ = status::StatusOK;
}

status::StatusCode Mixer::init_status() const {
    return init_status_;
}

bool Mixer::has_input(IFrameReader& reader) {
    roc_panic_if(init_status_ != status::StatusOK);

    for (size_t ni = 0; ni < inputs_.size(); ni++) {
        if (inputs_[ni].reader == &reader) {
            return true;
        }
    }

    return false;
}

ROC_ATTR_NODISCARD status::StatusCode Mixer::add_input(IFrameReader& reader) {
    roc_panic_if(init_status_ != status::StatusOK);

    Input input;
    input.reader = &reader;

    if (!inputs_.push_back(input)) {
        roc_log(LogError, "mixer: can't add input: allocation failed");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

void Mixer::remove_input(IFrameReader& reader) {
    roc_panic_if(init_status_ != status::StatusOK);

    size_t max_mixed = 0;
    size_t rm_idx = (size_t)-1;

    for (size_t ni = 0; ni < inputs_.size(); ni++) {
        Input& input = inputs_[ni];

        if (input.reader == &reader) {
            rm_idx = ni;
            continue;
        }

        max_mixed = std::max(max_mixed, input.n_mixed);
    }

    if (rm_idx == (size_t)-1) {
        roc_panic("mixer: can't remove input: reader not found");
    }

    // Zeroise removed samples.
    if (inputs_[rm_idx].n_mixed > max_mixed) {
        memset(mix_buffer_.data() + max_mixed, 0,
               (inputs_[rm_idx].n_mixed - max_mixed) * sizeof(sample_t));
    }

    // Remove from array.
    for (size_t ni = rm_idx + 1; ni < inputs_.size(); ni++) {
        inputs_[ni - 1] = inputs_[ni];
    }

    if (!inputs_.resize(inputs_.size() - 1)) {
        roc_panic("mixer: can't remove input: resize failed");
    }
}

status::StatusCode
Mixer::read(Frame& out_frame, packet::stream_timestamp_t duration, FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    // If frame doesn't have a buffer, or it's too small for @p duration,
    // allocate and attach buffer.
    if (!frame_factory_.reallocate_frame(
            out_frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    out_frame.set_raw(true);

    size_t out_size = duration * sample_spec_.num_channels();
    core::nanoseconds_t out_cts = 0;

    const status::StatusCode code =
        mix_all_repeat_(out_frame.raw_samples(), out_size, out_cts, mode);

    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    out_frame.set_capture_timestamp(out_cts);
    out_frame.set_num_raw_samples(out_size);
    out_frame.set_duration(packet::stream_timestamp_t(out_size)
                           / sample_spec_.num_channels());

    return code;
}

status::StatusCode Mixer::mix_all_repeat_(sample_t* out_data,
                                          size_t& out_size,
                                          core::nanoseconds_t& out_cts,
                                          FrameReadMode mode) {
    // Requested output frame size may be bigger than maximum size of mix buffer,
    // so we may need to repeat reading and mixing until output is fully filled.
    size_t out_pos = 0;

    while (out_pos < out_size) {
        size_t mix_batch_size = std::min(out_size - out_pos, mix_buffer_.size());
        core::nanoseconds_t mix_cts = 0;

        const status::StatusCode code =
            mix_all_(out_data + out_pos, mix_batch_size, mix_cts, mode);

        if (code == status::StatusDrain) {
            // Soft read stopped early.
            break;
        }

        if (code != status::StatusOK && code != status::StatusPart) {
            // Pipeline failure.
            return code;
        }

        if (out_pos == 0) {
            out_cts = mix_cts;
        }
        out_pos += mix_batch_size;

        if (code == status::StatusPart) {
            // Soft read stopped early.
            break;
        }
    }

    roc_panic_if(out_pos > out_size);

    if (out_pos == 0) {
        // Can happen only in soft read mode.
        roc_panic_if(mode != ModeSoft);
        return status::StatusDrain;
    }

    if (out_pos < out_size) {
        // Can happen only in soft read mode.
        roc_panic_if(mode != ModeSoft);
        out_size = out_pos;
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode Mixer::mix_all_(sample_t* out_data,
                                   size_t& out_size,
                                   core::nanoseconds_t& out_cts,
                                   FrameReadMode mode) {
    const size_t n_inputs = inputs_.size();

    roc_panic_if(!out_data);
    roc_panic_if(out_size > mix_buffer_.size());

    // When there are no inputs, produce silence.
    if (n_inputs == 0) {
        if (mode == ModeHard) {
            memset(out_data, 0, out_size * sizeof(sample_t));
            return status::StatusOK;
        } else {
            return status::StatusDrain;
        }
    }

    // Mix all inputs into mix buffer.
    sample_t* mix_data = mix_buffer_.data();
    const size_t mix_size = out_size;

    core::nanoseconds_t cts_base = 0;
    double cts_sum = 0;
    size_t cts_count = 0;

    size_t min_mix_size = 0;
    size_t max_mix_size = 0;

    for (size_t ni = 0; ni < n_inputs; ni++) {
        Input& input = inputs_[ni];

        // Read samples from input and mix them into mix buffer.
        // Each input tracks how much samples it already added to mix buffer,
        // and will only samples remaining up to requested size.
        const status::StatusCode code = mix_one_(input, mix_data, mix_size, mode);

        if (code != status::StatusOK && code != status::StatusPart
            && code != status::StatusDrain) {
            return code;
        }

        if (ni == 0) {
            min_mix_size = input.n_mixed;
        } else {
            min_mix_size = std::min(min_mix_size, input.n_mixed);
        }
        max_mix_size = std::max(max_mix_size, input.n_mixed);

        if (enable_timestamps_ && input.n_mixed != 0 && input.cts != 0) {
            // Subtract first non-zero timestamp from all other timestamps.
            // Since timestamp calculation is used only when inputs are synchronous
            // and their timestamps are close, this effectively makes all values
            // small, avoiding overflow and rounding errors when adding them.
            if (cts_base == 0) {
                cts_base = input.cts;
            }
            cts_sum += double(input.cts - cts_base);
            cts_count++;
        }
    }

    if (cts_count != 0) {
        // Compute average timestamp.
        // Don't forget to compensate everything that we subtracted above.
        out_cts = core::nanoseconds_t(cts_base * ((double)cts_count / n_inputs)
                                      + cts_sum / (double)n_inputs);
    }

    // At this point, min_mix_size refers to minimum position in mix buffer
    // that has samples from all inputs, and max_mix_size refers to maximum
    // position that has samples from at least one input.
    //
    // In soft read mode, these positions may be different because each input
    // may return different amount of samples.
    //
    // Below we return first min_mix_size samples to user and shift remaining
    // samples from min_mix_size to max_mix_size to the beginning of mix buffer.

    if (min_mix_size != 0) {
        // Copy mixed samples to output frame.
        memcpy(out_data, mix_data, min_mix_size * sizeof(sample_t));

        // Shift mixed samples to beginning of mix buffer.
        if (min_mix_size < max_mix_size) {
            memmove(mix_data, mix_data + min_mix_size,
                    (max_mix_size - min_mix_size) * sizeof(sample_t));
        }
        for (size_t ni = 0; ni < n_inputs; ni++) {
            Input& input = inputs_[ni];

            input.n_mixed -= min_mix_size;
            if (input.cts != 0) {
                input.cts += sample_spec_.samples_overall_2_ns(min_mix_size);
            }
        }

        // Zeroise shifted samples.
        memset(mix_data + (max_mix_size - min_mix_size), 0,
               min_mix_size * sizeof(sample_t));
    }

    roc_panic_if(min_mix_size > out_size);

    if (min_mix_size == 0) {
        return status::StatusDrain;
    }

    if (min_mix_size < out_size) {
        out_size = min_mix_size;
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode
Mixer::mix_one_(Input& input, sample_t* mix_data, size_t mix_size, FrameReadMode mode) {
    roc_panic_if(input.n_mixed % sample_spec_.num_channels() != 0);
    roc_panic_if(mix_size % sample_spec_.num_channels() != 0);

    // If input returned StatusFinish, don't call it anymore.
    if (input.is_finished && input.n_mixed < mix_size) {
        input.n_mixed = mix_size;
    }

    // Pipeline elements are allowed to return less samples than requested. In case
    // of partial read (StatusPart), we automatically repeat read for remaining samples.
    // We stop when one of the following happens:
    //   - we have fully filled requested buffer
    //   - we got StatusDrain, which means that soft read stopped early
    //   - we got StatusFinish, which means that reader is terminating
    //   - we got an error (any other status), which means that the whole mixer fails
    while (input.n_mixed < mix_size) {
        const packet::stream_timestamp_t remained_duration = packet::stream_timestamp_t(
            (mix_size - input.n_mixed) / sample_spec_.num_channels());

        const packet::stream_timestamp_t capped_duration =
            sample_spec_.cap_frame_duration(remained_duration,
                                            frame_factory_.byte_buffer_size());

        if (!frame_factory_.reallocate_frame(
                *in_frame_, sample_spec_.stream_timestamp_2_bytes(capped_duration))) {
            return status::StatusNoMem;
        }

        const status::StatusCode code =
            input.reader->read(*in_frame_, capped_duration, mode);

        if (code == status::StatusFinish) {
            // Stream ended and will be removed soon, pad it with zeros until that.
            input.n_mixed = mix_size;
            input.is_finished = true;
            break;
        }

        if (code == status::StatusDrain) {
            // Soft read stopped early.
            roc_panic_if_msg(mode != ModeSoft,
                             "mixer: unexpected drained read in hard-read mode");
            break;
        }

        if (code != status::StatusOK && code != status::StatusPart) {
            // Pipeline failure.
            roc_log(LogError, "mixer: can't read frame: status=%s",
                    status::code_to_str(code));
            return code;
        }

        sample_spec_.validate_frame(*in_frame_);

        // Mix samples.
        const size_t in_size = in_frame_->num_raw_samples();
        const sample_t* in_samples = in_frame_->raw_samples();
        sample_t* mix_samples = mix_data + input.n_mixed;

        for (size_t n = 0; n < in_size; n++) {
            *mix_samples += *in_samples;

            *mix_samples = std::min(*mix_samples, Sample_Max);
            *mix_samples = std::max(*mix_samples, Sample_Min);

            mix_samples++;
            in_samples++;
        }

        // Interpolate CTS of the first sample in mix buffer.
        core::nanoseconds_t in_cts = in_frame_->capture_timestamp();
        if (in_cts > 0) {
            in_cts -= sample_spec_.samples_overall_2_ns(input.n_mixed);
        }
        if (in_cts > 0) {
            input.cts = in_cts;
        } else {
            input.cts = 0;
        }

        input.n_mixed += in_size;
    }

    if (input.n_mixed == 0) {
        // Soft read stopped early.
        roc_panic_if(mode != ModeSoft);
        return status::StatusDrain;
    }

    if (input.n_mixed < mix_size) {
        // Soft read stopped early.
        roc_panic_if(mode != ModeSoft);
        return status::StatusPart;
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
