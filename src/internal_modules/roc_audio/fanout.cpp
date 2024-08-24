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
#include "roc_status/code_to_str.h"

namespace roc {
namespace audio {

Fanout::Fanout(const SampleSpec& sample_spec,
               FrameFactory& frame_factory,
               core::IArena& arena)
    : outputs_(arena)
    , sample_spec_(sample_spec)
    , init_status_(status::NoStatus) {
    roc_panic_if_msg(!sample_spec_.is_complete(),
                     "fanout: required complete sample spec: %s",
                     sample_spec_to_str(sample_spec_).c_str());

    init_status_ = status::StatusOK;
}

status::StatusCode Fanout::init_status() const {
    return init_status_;
}

bool Fanout::has_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    for (size_t no = 0; no < outputs_.size(); no++) {
        if (outputs_[no].writer == &writer) {
            return true;
        }
    }

    return false;
}

status::StatusCode Fanout::add_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    Output output;
    output.writer = &writer;

    if (!outputs_.push_back(output)) {
        roc_log(LogError, "fanout: can't add output: allocation failed");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

void Fanout::remove_output(IFrameWriter& writer) {
    roc_panic_if(init_status_ != status::StatusOK);

    size_t rm_idx = (size_t)-1;

    for (size_t no = 0; no < outputs_.size(); no++) {
        if (outputs_[no].writer == &writer) {
            rm_idx = no;
            break;
        }
    }

    if (rm_idx == (size_t)-1) {
        roc_panic("fanout: can't remove output: writer not found");
    }

    // Remove from array.
    for (size_t no = rm_idx + 1; no < outputs_.size(); no++) {
        outputs_[no - 1] = outputs_[no];
    }

    if (!outputs_.resize(outputs_.size() - 1)) {
        roc_panic("fanout: can't remove output: resize failed");
    }
}

status::StatusCode Fanout::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    sample_spec_.validate_frame(in_frame);

    for (size_t no = 0; no < outputs_.size(); no++) {
        Output& output = outputs_[no];

        if (output.is_finished) {
            continue;
        }

        const status::StatusCode code = output.writer->write(in_frame);

        if (code == status::StatusFinish) {
            // From now on, skip this writer until it's removed.
            output.is_finished = true;
            continue;
        }

        if (code != status::StatusOK) {
            // These codes can be returned only from read().
            roc_panic_if_msg(code == status::StatusPart || code == status::StatusDrain,
                             "fanout: unexpected status from write operation: status=%s",
                             status::code_to_str(code));
            return code;
        }
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
