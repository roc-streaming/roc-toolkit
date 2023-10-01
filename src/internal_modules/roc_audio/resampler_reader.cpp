/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_reader.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ResamplerReader::ResamplerReader(IFrameReader& reader,
                                 IResampler& resampler,
                                 const SampleSpec& in_sample_spec,
                                 const SampleSpec& out_sample_spec)
    : resampler_(resampler)
    , reader_(reader)
    , in_sample_spec_(in_sample_spec)
    , out_sample_spec_(out_sample_spec)
    , last_in_cts_(0)
    , scaling_(1.0f)
    , valid_(false) {
    if (in_sample_spec_.channel_set() != out_sample_spec_.channel_set()) {
        roc_panic("resampler reader: input and output channel sets should be same");
    }

    if (!resampler_.is_valid()) {
        return;
    }

    if (!resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                out_sample_spec_.sample_rate(), 1.0f)) {
        return;
    }

    valid_ = true;
}

bool ResamplerReader::is_valid() const {
    return valid_;
}

bool ResamplerReader::set_scaling(float multiplier) {
    roc_panic_if_not(is_valid());

    scaling_ = multiplier;

    return resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                  out_sample_spec_.sample_rate(), multiplier);
}

bool ResamplerReader::read(Frame& out_frame) {
    roc_panic_if_not(is_valid());

    if (out_frame.num_samples() % out_sample_spec_.num_channels() != 0) {
        roc_panic("resampler reader: unexpected frame size");
    }

    size_t out_pos = 0;

    while (out_pos < out_frame.num_samples()) {
        Frame out_part(out_frame.samples() + out_pos, out_frame.num_samples() - out_pos);

        const size_t num_popped = resampler_.pop_output(out_part);

        if (num_popped < out_part.num_samples()) {
            if (!push_input_()) {
                return false;
            }
        }

        out_pos += num_popped;
    }

    out_frame.set_capture_timestamp(capture_ts_(out_frame));

    return true;
}

bool ResamplerReader::push_input_() {
    const core::Slice<sample_t>& in_buff = resampler_.begin_push_input();

    Frame in_frame(in_buff.data(), in_buff.size());

    if (!reader_.read(in_frame)) {
        return false;
    }

    resampler_.end_push_input();

    const core::nanoseconds_t in_cts = in_frame.capture_timestamp();

    if (in_cts > 0) {
        // Remember timestamp of last sample of last input frame.
        last_in_cts_ =
            in_cts + in_sample_spec_.samples_overall_2_ns(in_frame.num_samples());
    }

    return true;
}

// Compute timestamp of first sample of current output frame.
// We have timestamps in input frames, and we should find to
// which time our output frame does correspond in input stream.
core::nanoseconds_t ResamplerReader::capture_ts_(Frame& out_frame) {
    if (last_in_cts_ == 0) {
        // We didn't receive input frame with non-zero cts yet,
        // so for now we keep cts zero.
        return 0;
    }

    // Get timestamp of last sample of last input frame pushed to resampler.
    // Now we have tail of input stream.
    core::nanoseconds_t out_cts = last_in_cts_;

    // Subtract number of input samples that resampler haven't processed yet.
    // Now we have point in input stream corresponding to tail of output frame.
    out_cts -=
        in_sample_spec_.fract_samples_per_chan_2_ns(resampler_.n_left_to_process());

    // Subtract length of current output frame multiplied by scaling.
    // Now we have point in input stream corresponding to head of output frame.
    out_cts -= core::nanoseconds_t(
        out_sample_spec_.samples_overall_2_ns(out_frame.num_samples()) * scaling_);

    if (out_cts < 0) {
        // Input frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts until it grows a bit.
        return 0;
    }

    return out_cts;
}

} // namespace audio
} // namespace roc
