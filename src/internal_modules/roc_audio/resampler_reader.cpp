/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_reader.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerReader::ResamplerReader(IReader& reader, IResampler& resampler)
    : resampler_(resampler)
    , reader_(reader) {
}

bool ResamplerReader::valid() const {
    return resampler_.valid();
}

bool ResamplerReader::set_scaling(size_t input_rate, size_t output_rate, float mult) {
    roc_panic_if_not(valid());

    return resampler_.set_scaling(input_rate, output_rate, mult);
}

bool ResamplerReader::read(Frame& out) {
    roc_panic_if_not(valid());

    size_t out_pos = 0;

    while (out_pos < out.size()) {
        Frame out_part(out.data() + out_pos, out.size() - out_pos);

        const size_t num_popped = resampler_.pop_output(out_part);

        if (num_popped < out_part.size()) {
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
