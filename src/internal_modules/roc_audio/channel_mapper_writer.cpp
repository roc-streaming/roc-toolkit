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

ChannelMapperWriter::ChannelMapperWriter(IFrameWriter& frame_writer,
                                         FrameFactory& frame_factory,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : frame_factory_(frame_factory)
    , frame_writer_(frame_writer)
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("channel mapper writer: required complete sample specs with raw format:"
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

    roc_log(LogDebug, "channel mapper writer: initializing: in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());

    out_frame_ = frame_factory_.allocate_frame(0);
    if (!out_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode ChannelMapperWriter::init_status() const {
    return init_status_;
}

status::StatusCode ChannelMapperWriter::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    in_spec_.validate_frame(in_frame);

    const size_t in_size = in_frame.num_raw_samples();
    size_t in_pos = 0;

    while (in_pos < in_size) {
        const packet::stream_timestamp_t remained_duration =
            packet::stream_timestamp_t((in_size - in_pos) / in_spec_.num_channels());

        const packet::stream_timestamp_t capped_duration = out_spec_.cap_frame_duration(
            remained_duration, frame_factory_.byte_buffer_size());

        const size_t in_batch_size = capped_duration * in_spec_.num_channels();

        if (!frame_factory_.reallocate_frame(
                *out_frame_, out_spec_.stream_timestamp_2_bytes(capped_duration))) {
            return status::StatusNoMem;
        }

        out_frame_->set_flags(in_frame.flags());
        out_frame_->set_raw(true);
        out_frame_->set_duration(capped_duration);

        if (in_frame.capture_timestamp() != 0) {
            out_frame_->set_capture_timestamp(in_frame.capture_timestamp()
                                              + in_spec_.samples_overall_2_ns(in_pos));
        }

        mapper_.map(in_frame.raw_samples() + in_pos, in_batch_size,
                    out_frame_->raw_samples(), out_frame_->num_raw_samples());

        in_pos += in_batch_size;

        const status::StatusCode code = frame_writer_.write(*out_frame_);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
