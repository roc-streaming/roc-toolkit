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
    , valid_(false) {
    if (in_sample_spec_.channel_mask() != out_sample_spec_.channel_mask()) {
        roc_panic("resampler reader: input and output channel mask should be equal");
    }

    if (!resampler_.valid()) {
        return;
    }

    if (!resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                out_sample_spec_.sample_rate(), 1.0f)) {
        return;
    }

    valid_ = true;
}

bool ResamplerReader::valid() const {
    return valid_;
}

bool ResamplerReader::set_scaling(float multiplier) {
    roc_panic_if_not(valid());

    scaling_ = multiplier;

    return resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                  out_sample_spec_.sample_rate(), multiplier);
}

bool ResamplerReader::read(Frame& out) {
    roc_panic_if_not(valid());

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

    return true;
}

bool ResamplerReader::push_input_() {
    const core::Slice<sample_t>& buff = resampler_.begin_push_input();

    Frame frame(buff.data(), buff.size());

    if (!reader_.read(frame)) {
        return false;
    }

    resampler_.end_push_input();
    return true;
}

} // namespace audio
} // namespace roc
