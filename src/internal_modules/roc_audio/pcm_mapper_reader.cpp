/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/sample_format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmMapperReader::PcmMapperReader(IFrameReader& reader,
                                 FrameFactory& frame_factory,
                                 const SampleSpec& in_spec,
                                 const SampleSpec& out_spec)
    : mapper_(in_spec.pcm_format(), out_spec.pcm_format())
    , in_reader_(reader)
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , num_ch_(out_spec.num_channels())
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_valid() || !out_spec_.is_valid()
        || in_spec_.sample_format() != SampleFormat_Pcm
        || out_spec_.sample_format() != SampleFormat_Pcm) {
        roc_panic("pcm mapper reader: required valid sample specs with pcm format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.sample_rate() != out_spec_.sample_rate()
        || in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic(
            "pcm mapper reader: required identical input and output rates and channels:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec).c_str(), sample_spec_to_str(out_spec).c_str());
    }

    if (mapper_.input_bit_count(1) % 8 != 0 || mapper_.output_bit_count(1) % 8 != 0) {
        roc_panic("pcm mapper reader: unsupported not byte-aligned encoding:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    in_buf_ = frame_factory.new_byte_buffer();
    if (!in_buf_) {
        roc_log(LogError, "pcm mapper reader: can't allocate temporary buffer");
        init_status_ = status::StatusNoMem;
        return;
    }
    in_buf_.reslice(0, in_buf_.capacity());

    init_status_ = status::StatusOK;
}

status::StatusCode PcmMapperReader::init_status() const {
    return init_status_;
}

status::StatusCode PcmMapperReader::read(Frame& out_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    const size_t max_sample_count = mapper_.input_sample_count(in_buf_.size()) / num_ch_;

    const size_t out_sample_count =
        mapper_.output_sample_count(out_frame.num_bytes()) / num_ch_;
    size_t out_sample_offset = 0;

    const size_t out_bit_count = mapper_.output_bit_count(out_sample_count * num_ch_);
    size_t out_bit_offset = 0;

    unsigned out_flags = 0;

    while (out_bit_offset < out_bit_count) {
        const size_t n_samples =
            std::min(out_sample_count - out_sample_offset, max_sample_count);

        const size_t in_byte_count = mapper_.input_byte_count(n_samples * num_ch_);
        size_t in_bit_offset = 0;

        Frame in_frame(in_buf_.data(), in_byte_count);

        const status::StatusCode code = in_reader_.read(in_frame);
        if (code != status::StatusOK) {
            return code;
        }

        mapper_.map(in_buf_.data(), in_byte_count, in_bit_offset, out_frame.bytes(),
                    out_frame.num_bytes(), out_bit_offset, n_samples * num_ch_);

        out_flags |= in_frame.flags();
        if (out_sample_offset == 0) {
            out_frame.set_capture_timestamp(in_frame.capture_timestamp());
        }

        out_sample_offset += n_samples;
    }

    if (out_spec_.is_raw()) {
        out_flags &= ~(unsigned)Frame::HasEncoding;
    } else {
        out_flags |= (unsigned)Frame::HasEncoding;
    }

    out_frame.set_flags(out_flags);
    out_frame.set_duration(out_sample_count);

    return status::StatusOK;
}

} // namespace audio
} // namespace roc
