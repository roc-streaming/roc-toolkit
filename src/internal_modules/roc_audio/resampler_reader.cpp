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
    , scaling_(1.0f)
    , valid_(false)
    , last_in_ts_(0) {
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

bool ResamplerReader::read(Frame& out) {
    roc_panic_if_not(is_valid());

    if (out.num_samples() % out_sample_spec_.num_channels() != 0) {
        roc_panic("resampler reader: unexpected frame size");
    }

    size_t out_pos = 0;

    while (out_pos < out.num_samples()) {
        Frame out_part(out.samples() + out_pos, out.num_samples() - out_pos);

        const size_t num_popped = resampler_.pop_output(out_part);

        if (num_popped < out_part.num_samples()) {
            if (!push_input_()) {
                return false;
            }
        }

        out_pos += num_popped;
    }

    out.set_capture_timestamp(capture_ts_(out));

    return true;
}

bool ResamplerReader::push_input_() {
    const core::Slice<sample_t>& buff = resampler_.begin_push_input();

    Frame frame(buff.data(), buff.size());

    if (!reader_.read(frame)) {
        return false;
    }

    resampler_.end_push_input();

    const core::nanoseconds_t capt_ts = frame.capture_timestamp();
    if (capt_ts) {
        last_in_ts_ = capt_ts + in_sample_spec_.samples_overall_2_ns(frame.num_samples());
    }

    return true;
}

core::nanoseconds_t ResamplerReader::capture_ts_(Frame& frame) {
    if (last_in_ts_ == 0) {
        // we didn't receive frame with non-zero cts yet
        return 0;
    }

    const core::nanoseconds_t capt_ts = last_in_ts_
        - in_sample_spec_.fract_samples_per_chan_2_ns(resampler_.n_left_to_process())
        - core::nanoseconds_t(out_sample_spec_.samples_overall_2_ns(frame.num_samples())
                              * scaling_);

    if (capt_ts < 0) {
        // frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts
        return 0;
    }

    return capt_ts;
}

} // namespace audio
} // namespace roc
