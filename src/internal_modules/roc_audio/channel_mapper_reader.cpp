/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_reader.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapperReader::ChannelMapperReader(IFrameReader& reader,
                                         FrameFactory& frame_factory,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : input_reader_(reader)
    , input_buf_()
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , valid_(false) {
    if (!in_spec_.is_valid() || !out_spec_.is_valid() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("channel mapper reader: required valid sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper reader: required identical input and output rates:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    input_buf_ = frame_factory.new_raw_buffer();
    if (!input_buf_) {
        roc_log(LogError, "channel mapper reader: can't allocate temporary buffer");
        return;
    }

    input_buf_.reslice(0, input_buf_.capacity());

    valid_ = true;
}

bool ChannelMapperReader::is_valid() const {
    return valid_;
}

bool ChannelMapperReader::read(Frame& out_frame) {
    roc_panic_if(!valid_);

    if (out_frame.num_raw_samples() % out_spec_.num_channels() != 0) {
        roc_panic("channel mapper reader: unexpected frame size");
    }

    const size_t max_batch = input_buf_.size() / in_spec_.num_channels();

    sample_t* out_samples = out_frame.raw_samples();
    size_t n_samples = out_frame.num_raw_samples() / out_spec_.num_channels();

    unsigned flags = 0;

    size_t frames_counter = 0;
    while (n_samples != 0) {
        const size_t n_read = std::min(n_samples, max_batch);

        core::nanoseconds_t capt_ts = 0;
        if (!read_(out_samples, n_read, flags, capt_ts)) {
            return false;
        }

        if (frames_counter == 0) {
            out_frame.set_capture_timestamp(capt_ts);
        }
        frames_counter++;
        out_samples += n_read * out_spec_.num_channels();
        n_samples -= n_read;
    }

    out_frame.set_flags(flags);
    out_frame.set_duration(out_frame.num_raw_samples() / out_spec_.num_channels());

    return true;
}

bool ChannelMapperReader::read_(sample_t* out_samples,
                                size_t n_samples,
                                unsigned& flags,
                                core::nanoseconds_t& capt_ts) {
    Frame in_frame(input_buf_.data(), n_samples * in_spec_.num_channels());
    if (!input_reader_.read(in_frame)) {
        return false;
    }

    mapper_.map(in_frame.raw_samples(), in_frame.num_raw_samples(), out_samples,
                n_samples * out_spec_.num_channels());

    capt_ts = in_frame.capture_timestamp();
    flags |= in_frame.flags();

    return true;
}

} // namespace audio
} // namespace roc
