/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_mapper_writer.h"
#include "roc_audio/format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmMapperWriter::PcmMapperWriter(IFrameWriter& frame_writer,
                                 FrameFactory& frame_factory,
                                 const SampleSpec& in_spec,
                                 const SampleSpec& out_spec)
    : frame_factory_(frame_factory)
    , frame_writer_(frame_writer)
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , num_ch_(out_spec.num_channels())
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete()
        || in_spec_.format() != Format_Pcm || out_spec_.format() != Format_Pcm) {
        roc_panic("pcm mapper writer: required complete sample specs with pcm format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()
        || in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic(
            "pcm mapper writer: required identical input and output rates and channels:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());
    }

    if (!in_spec_.is_raw() && !out_spec_.is_raw()) {
        roc_panic(
            "pcm mapper writer: required either input our output spec to have raw format:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());
    }

    mapper_.reset(new (mapper_)
                      PcmMapper(in_spec_.pcm_subformat(), out_spec_.pcm_subformat()));

    if (mapper_->input_bit_count(1) % 8 != 0 || mapper_->output_bit_count(1) % 8 != 0) {
        roc_panic("pcm mapper writer: unsupported not byte-aligned encoding:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    roc_log(LogDebug, "pcm mapper writer: initializing: in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec_).c_str(), sample_spec_to_str(out_spec_).c_str());

    out_frame_ = frame_factory_.allocate_frame(0);
    if (!out_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode PcmMapperWriter::init_status() const {
    return init_status_;
}

status::StatusCode PcmMapperWriter::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    in_spec_.validate_frame(in_frame);

    const size_t in_size = in_frame.num_bytes();
    size_t in_pos = 0;

    while (in_pos < in_size) {
        const packet::stream_timestamp_t remained_duration =
            in_spec_.bytes_2_stream_timestamp(in_size - in_pos);

        const packet::stream_timestamp_t capped_duration = out_spec_.cap_frame_duration(
            remained_duration, frame_factory_.byte_buffer_size());

        if (!frame_factory_.reallocate_frame(
                *out_frame_, out_spec_.stream_timestamp_2_bytes(capped_duration))) {
            return status::StatusNoMem;
        }

        out_frame_->set_flags(in_frame.flags());
        out_frame_->set_raw(out_spec_.is_raw());
        out_frame_->set_duration(capped_duration);

        if (in_frame.capture_timestamp() != 0) {
            out_frame_->set_capture_timestamp(in_frame.capture_timestamp()
                                              + in_spec_.bytes_2_ns(in_pos));
        }

        const size_t out_byte_count =
            mapper_->output_byte_count(capped_duration * num_ch_);
        size_t out_bit_offset = 0;

        const size_t in_byte_count = mapper_->input_byte_count(capped_duration * num_ch_);
        size_t in_bit_offset = 0;

        mapper_->map(in_frame.bytes() + in_pos, in_byte_count, in_bit_offset,
                     out_frame_->bytes(), out_byte_count, out_bit_offset,
                     capped_duration * num_ch_);

        roc_panic_if(out_bit_offset != out_byte_count * 8);
        roc_panic_if(in_bit_offset != in_byte_count * 8);

        in_pos += in_byte_count;

        const status::StatusCode code = frame_writer_.write(*out_frame_);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
