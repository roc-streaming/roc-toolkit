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

ChannelMapperReader::ChannelMapperReader(IFrameReader& frame_reader,
                                         FrameFactory& frame_factory,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : frame_factory_(frame_factory)
    , frame_reader_(frame_reader)
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("channel mapper reader: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper reader: required identical input and output rates:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    roc_log(LogDebug, "channel mapper reader: initializing: in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());

    in_frame_ = frame_factory_.allocate_frame(0);
    if (!in_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode ChannelMapperReader::init_status() const {
    return init_status_;
}

status::StatusCode ChannelMapperReader::read(
    Frame& out_frame, packet::stream_timestamp_t requested_duration, FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    packet::stream_timestamp_t capped_duration = out_spec_.cap_frame_duration(
        requested_duration, frame_factory_.byte_buffer_size());

    capped_duration =
        in_spec_.cap_frame_duration(capped_duration, frame_factory_.byte_buffer_size());

    if (!frame_factory_.reallocate_frame(
            *in_frame_, in_spec_.stream_timestamp_2_bytes(capped_duration))) {
        return status::StatusNoMem;
    }

    const status::StatusCode code = frame_reader_.read(*in_frame_, capped_duration, mode);
    if (code != status::StatusOK && code != status::StatusPart) {
        return code;
    }

    in_spec_.validate_frame(*in_frame_);

    const packet::stream_timestamp_t resulted_duration = in_frame_->duration();

    if (!frame_factory_.reallocate_frame(
            out_frame, out_spec_.stream_timestamp_2_bytes(resulted_duration))) {
        return status::StatusNoMem;
    }

    out_frame.set_flags(in_frame_->flags());
    out_frame.set_raw(true);
    out_frame.set_duration(resulted_duration);
    out_frame.set_capture_timestamp(in_frame_->capture_timestamp());

    mapper_.map(in_frame_->raw_samples(), in_frame_->num_raw_samples(),
                out_frame.raw_samples(), out_frame.num_raw_samples());

    return resulted_duration == requested_duration ? status::StatusOK
                                                   : status::StatusPart;
}

} // namespace audio
} // namespace roc
