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
#include "roc_packet/ntp.h"

namespace roc {
namespace audio {

ChannelMapperWriter::ChannelMapperWriter(IFrameWriter& writer,
                                         core::BufferFactory<sample_t>& buffer_factory,
                                         const SampleSpec& in_spec,
                                         const SampleSpec& out_spec)
    : output_writer_(writer)
    , output_buf_()
    , mapper_(in_spec.channel_set(), out_spec.channel_set())
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , valid_(false) {
    if (in_spec_.sample_rate() != out_spec_.sample_rate()) {
        roc_panic("channel mapper writer: input and output sample rate should be equal:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    output_buf_ = buffer_factory.new_buffer();
    if (!output_buf_) {
        roc_log(LogError, "channel mapper writer: can't allocate temporary buffer");
        return;
    }

    output_buf_.reslice(0, output_buf_.capacity());

    valid_ = true;
}

bool ChannelMapperWriter::is_valid() const {
    return valid_;
}

void ChannelMapperWriter::write(Frame& in_frame) {
    roc_panic_if(!valid_);

    if (in_frame.num_samples() % in_spec_.num_channels() != 0) {
        roc_panic("channel mapper writer: unexpected frame size");
    }

    const size_t max_batch = output_buf_.size() / out_spec_.num_channels();

    sample_t* in_samples = in_frame.samples();
    size_t n_samples = in_frame.num_samples() / in_spec_.num_channels();

    const unsigned flags = in_frame.flags();

    while (n_samples != 0) {
        const size_t n_write = std::min(n_samples, max_batch);

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
    out_frame.capture_timestamp() = core::timestamp(core::ClockUnix);

    output_writer_.write(out_frame);
}

} // namespace audio
} // namespace roc
