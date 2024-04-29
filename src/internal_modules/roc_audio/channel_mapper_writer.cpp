/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapperWriter::ChannelMapperWriter(IFrameWriter& writer,
                                         FrameFactory& frame_factory,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : output_writer_(writer)
    , output_buf_()
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_valid() || !out_spec_.is_valid() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("channel mapper writer: required valid sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper writer: required identical input and output rates:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    output_buf_ = frame_factory.new_raw_buffer();
    if (!output_buf_) {
        roc_log(LogError, "channel mapper writer: can't allocate temporary buffer");
        init_status_ = status::StatusNoMem;
        return;
    }

    output_buf_.reslice(0, output_buf_.capacity());

    init_status_ = status::StatusOK;
}

status::StatusCode ChannelMapperWriter::init_status() const {
    return init_status_;
}

status::StatusCode ChannelMapperWriter::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (in_frame.num_raw_samples() % in_spec_.num_channels() != 0) {
        roc_panic("channel mapper writer: unexpected frame size");
    }

    const size_t max_batch = output_buf_.size() / out_spec_.num_channels();

    core::nanoseconds_t capt_ts = in_frame.capture_timestamp();
    sample_t* in_samples = in_frame.raw_samples();
    size_t n_samples = in_frame.num_raw_samples() / in_spec_.num_channels();

    const unsigned flags = in_frame.flags();

    while (n_samples != 0) {
        const size_t n_write = std::min(n_samples, max_batch);

        const status::StatusCode code = write_(in_samples, n_write, flags, capt_ts);
        if (code != status::StatusOK) {
            return code;
        }

        in_samples += n_write * in_spec_.num_channels();
        n_samples -= n_write;

        if (capt_ts) {
            capt_ts += in_spec_.samples_per_chan_2_ns(n_write);
        }
    }

    return status::StatusOK;
}

status::StatusCode ChannelMapperWriter::write_(sample_t* in_samples,
                                               size_t n_samples,
                                               unsigned flags,
                                               core::nanoseconds_t capture_ts) {
    Frame out_frame(output_buf_.data(), n_samples * out_spec_.num_channels());

    mapper_.map(in_samples, n_samples * in_spec_.num_channels(), out_frame.raw_samples(),
                out_frame.num_raw_samples());

    out_frame.set_flags(flags);
    out_frame.set_duration(out_frame.num_raw_samples() / out_spec_.num_channels());
    out_frame.set_capture_timestamp(capture_ts);

    return output_writer_.write(out_frame);
}

} // namespace audio
} // namespace roc
