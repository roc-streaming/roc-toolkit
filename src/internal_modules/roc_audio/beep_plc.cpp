/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/beep_plc.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

BeepPlc::BeepPlc(const PlcConfig& config,
                 const SampleSpec& sample_spec,
                 FrameFactory& frame_factory,
                 core::IArena& arena)
    : IPlc(arena)
    , sample_spec_(sample_spec)
    , signal_pos_(0) {
    if (!sample_spec_.is_complete() || !sample_spec_.is_raw()) {
        roc_panic("beep plc: required complete sample specs with raw format: spec=%s",
                  sample_spec_to_str(sample_spec_).c_str());
    }
}

BeepPlc::~BeepPlc() {
}

status::StatusCode BeepPlc::init_status() const {
    return status::StatusOK;
}

SampleSpec BeepPlc::sample_spec() const {
    return sample_spec_;
}

packet::stream_timestamp_t BeepPlc::lookbehind_len() {
    return 0;
}

packet::stream_timestamp_t BeepPlc::lookahead_len() {
    return 0;
}

void BeepPlc::process_history(Frame& hist_frame) {
    sample_spec_.validate_frame(hist_frame);

    signal_pos_ += hist_frame.duration();
}

void BeepPlc::process_loss(Frame& lost_frame, Frame* prev_frame, Frame* next_frame) {
    sample_spec_.validate_frame(lost_frame);

    sample_t* lost_samples = lost_frame.raw_samples();
    const size_t lost_samples_count =
        lost_frame.num_raw_samples() / sample_spec_.num_channels();

    for (size_t ns = 0; ns < lost_samples_count; ns++) {
        const sample_t s = (sample_t)std::sin(2.0 * M_PI / sample_spec_.sample_rate()
                                              * 880.0 * signal_pos_++);

        for (size_t nc = 0; nc < sample_spec_.num_channels(); nc++) {
            *lost_samples++ = s;
        }
    }
}

} // namespace audio
} // namespace roc
