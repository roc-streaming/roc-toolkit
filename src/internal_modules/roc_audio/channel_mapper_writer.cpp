/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapperWriter::ChannelMapperWriter(IFrameWriter& writer,
                                         core::BufferFactory<sample_t>& buffer_factory,
                                         core::nanoseconds_t frame_length,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : output_writer_(writer)
    , output_buf_()
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , mapper_enabled_(in_spec.channel_set() != out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , valid_(false) {
    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper writer: input and output sample rate should be equal");
    }

    if (mapper_enabled_) {
        const size_t frame_size = out_spec.ns_2_samples_overall(frame_length);
        roc_log(LogDebug,
                "channel mapper writer:"
                " initializing: frame_size=%lu in_chans=%s out_chans=%s",
                (unsigned long)frame_size,
                channel_set_to_str(in_spec.channel_set()).c_str(),
                channel_set_to_str(out_spec.channel_set()).c_str());

        if (frame_size == 0) {
            roc_log(LogError, "channel mapper writer: frame size cannot be 0");
            return;
        }

        output_buf_ = buffer_factory.new_buffer();
        if (!output_buf_) {
            roc_log(LogError, "channel mapper writer: can't allocate temporary buffer");
            return;
        }

        if (output_buf_.capacity() < frame_size) {
            roc_log(LogError, "channel mapper writer: allocated buffer is too small");
            return;
        }
        output_buf_.reslice(0, frame_size);
    }

    valid_ = true;
}

bool ChannelMapperWriter::valid() const {
    return valid_;
}

void ChannelMapperWriter::write(Frame& in_frame) {
    roc_panic_if(!valid_);

    if (in_frame.num_samples() % in_spec_.num_channels() != 0) {
        roc_panic("channel mapper writer: unexpected frame size");
    }

    if (!mapper_enabled_) {
        return output_writer_.write(in_frame);
    }

    const size_t max_batch = output_buf_.size() / out_spec_.num_channels();

    sample_t* in_samples = in_frame.samples();
    size_t n_samples = in_frame.num_samples() / in_spec_.num_channels();

    const unsigned flags = in_frame.flags();

    while (n_samples != 0) {
        const size_t n_write = std::max(n_samples, max_batch);

        write_(in_samples, n_write, flags);

        in_samples += n_write * in_spec_.num_channels();
        n_samples -= n_write;
    }
}

void ChannelMapperWriter::write_(sample_t* in_samples, size_t n_samples, unsigned flags) {
    Frame in_frame(in_samples, n_samples * in_spec_.num_channels());

    Frame out_frame(output_buf_.data(), n_samples * out_spec_.num_channels());

    out_frame.set_flags(flags);

    mapper_.map(in_frame, out_frame);

    output_writer_.write(out_frame);
}

} // namespace audio
} // namespace roc
