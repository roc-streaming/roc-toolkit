/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapperReader::ChannelMapperReader(IFrameReader& reader,
                                         core::BufferFactory<sample_t>& buffer_factory,
                                         core::nanoseconds_t frame_length,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : input_reader_(reader)
    , input_buf_()
    , mapper_(in_spec.channel_mask(), out_spec.channel_mask())
    , mapper_enabled_(in_spec.channel_mask() != out_spec.channel_mask())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , valid_(false) {
    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper reader: input and output sample rate should be equal");
    }

    if (mapper_enabled_) {
        const size_t frame_size = in_spec.ns_2_samples_overall(frame_length);
        roc_log(LogDebug,
                "channel mapper reader:"
                " initializing: frame_size=%lu in_mask=0x%lx out_mask=%lx",
                (unsigned long)frame_size, (unsigned long)in_spec.channel_mask(),
                (unsigned long)out_spec.channel_mask());

        if (frame_size == 0) {
            roc_log(LogError, "channel mapper reader: frame size cannot be 0");
            return;
        }

        input_buf_ = buffer_factory.new_buffer();
        if (!input_buf_) {
            roc_log(LogError, "channel mapper reader: can't allocate temporary buffer");
            return;
        }

        if (input_buf_.capacity() < frame_size) {
            roc_log(LogError, "channel mapper reader: allocated buffer is too small");
            return;
        }
        input_buf_.reslice(0, frame_size);
    }

    valid_ = true;
}

bool ChannelMapperReader::valid() const {
    return valid_;
}

bool ChannelMapperReader::read(Frame& out_frame) {
    roc_panic_if(!valid_);

    if (out_frame.num_samples() % out_spec_.num_channels() != 0) {
        roc_panic("channel mapper reader: unexpected frame size");
    }

    if (!mapper_enabled_) {
        return input_reader_.read(out_frame);
    }

    const size_t max_batch = input_buf_.size() / in_spec_.num_channels();

    sample_t* out_samples = out_frame.samples();
    size_t n_samples = out_frame.num_samples() / out_spec_.num_channels();

    unsigned flags = 0;

    while (n_samples != 0) {
        const size_t n_read = std::max(n_samples, max_batch);

        if (!read_(out_samples, n_read, flags)) {
            return false;
        }

        out_samples += n_read * out_spec_.num_channels();
        n_samples -= n_read;
    }

    out_frame.set_flags(flags);

    return true;
}

bool ChannelMapperReader::read_(sample_t* out_samples,
                                size_t n_samples,
                                unsigned& flags) {
    Frame out_frame(out_samples, n_samples * out_spec_.num_channels());

    Frame in_frame(input_buf_.data(), n_samples * in_spec_.num_channels());

    if (!input_reader_.read(in_frame)) {
        return false;
    }

    mapper_.map(in_frame, out_frame);

    flags |= in_frame.flags();

    return true;
}

} // namespace audio
} // namespace roc
