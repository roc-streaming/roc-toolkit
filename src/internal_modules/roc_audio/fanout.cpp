/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/fanout.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

Fanout::Fanout(const SampleSpec& sample_spec)
    : sample_spec_(sample_spec)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_valid() || !sample_spec_.is_raw(),
                     "fanout: required valid sample spec with raw format: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    init_status_ = status::StatusOK;
}

status::StatusCode Fanout::init_status() const {
    return init_status_;
}

bool Fanout::has_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    return frame_writers_.contains(writer);
}

void Fanout::add_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    frame_writers_.push_back(writer);
}

void Fanout::remove_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    frame_writers_.remove(writer);
}

status::StatusCode Fanout::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    sample_spec_.validate_frame(in_frame);

    for (IFrameWriter* writer = frame_writers_.front(); writer != NULL;
         writer = frame_writers_.nextof(*writer)) {
        const status::StatusCode code = writer->write(in_frame);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
