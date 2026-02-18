/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmMapperReader::PcmMapperReader(IFrameReader& frame_reader,
                                 FrameFactory& frame_factory,
                                 const SampleSpec& in_spec,
                                 const SampleSpec& out_spec)
    : frame_factory_(frame_factory)
    , frame_reader_(frame_reader)
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , num_ch_(out_spec.num_channels())
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete()
        || in_spec_.format() != Format_Pcm || out_spec_.format() != Format_Pcm) {
        roc_panic("pcm mapper reader: required complete sample specs with pcm format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (!in_spec_.is_raw() && !out_spec_.is_raw()) {
        roc_panic(
            "pcm mapper reader: required either input our output spec to have raw format:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()
        || in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic(
            "pcm mapper reader: required identical input and output rates and channels:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());
    }

    mapper_.reset(new (mapper_)
                      PcmMapper(in_spec_.pcm_subformat(), out_spec_.pcm_subformat()));

    if (mapper_->input_bit_count(1) % 8 != 0 || mapper_->output_bit_count(1) % 8 != 0) {
        roc_panic("pcm mapper reader: unsupported not byte-aligned encoding:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    roc_log(LogDebug, "pcm mapper reader: initializing: in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());

    in_frame_ = frame_factory_.allocate_frame(0);
    if (!in_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode PcmMapperReader::init_status() const {
    return init_status_;
}

status::StatusCode PcmMapperReader::read(Frame& out_frame,
                                         packet::stream_timestamp_t requested_duration,
                                         FrameReadMode mode) {
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
    out_frame.set_raw(out_spec_.is_raw());
    out_frame.set_duration(resulted_duration);
    out_frame.set_capture_timestamp(in_frame_->capture_timestamp());

    const size_t out_byte_count = mapper_->output_byte_count(resulted_duration * num_ch_);
    size_t out_bit_offset = 0;

    const size_t in_byte_count = mapper_->input_byte_count(resulted_duration * num_ch_);
    size_t in_bit_offset = 0;

    mapper_->map(in_frame_->bytes(), in_byte_count, in_bit_offset, out_frame.bytes(),
                 out_byte_count, out_bit_offset, resulted_duration * num_ch_);

    roc_panic_if(out_bit_offset != out_byte_count * 8);
    roc_panic_if(in_bit_offset != in_byte_count * 8);

    return resulted_duration == requested_duration ? status::StatusOK
                                                   : status::StatusPart;
}

} // namespace audio
} // namespace roc
