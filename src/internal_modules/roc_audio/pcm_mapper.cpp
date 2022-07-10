/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_mapper.h"
#include "roc_audio/pcm_mapper_func.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmMapper::PcmMapper(const PcmFormat& input_fmt, const PcmFormat& output_fmt)
    : input_fmt_(input_fmt)
    , output_fmt_(output_fmt)
    , input_sample_bits_(pcm_sample_bits(input_fmt.encoding))
    , output_sample_bits_(pcm_sample_bits(output_fmt.encoding))
    , map_func_(pcm_mapper_func(input_fmt_.encoding,
                                output_fmt_.encoding,
                                input_fmt_.endian,
                                output_fmt_.endian)) {
    if (!map_func_) {
        roc_panic("pcm mapper: unable to select mapper function");
    }
}

const PcmFormat& PcmMapper::input_format() const {
    return input_fmt_;
}

const PcmFormat& PcmMapper::output_format() const {
    return output_fmt_;
}

size_t PcmMapper::input_sample_count(size_t input_bytes) const {
    return input_bytes * 8 / input_sample_bits_;
}

size_t PcmMapper::output_sample_count(size_t output_bytes) const {
    return output_bytes * 8 / output_sample_bits_;
}

size_t PcmMapper::input_byte_count(size_t input_samples) const {
    return (input_samples * input_sample_bits_ + 7) / 8;
}

size_t PcmMapper::output_byte_count(size_t output_samples) const {
    return (output_samples * output_sample_bits_ + 7) / 8;
}

void PcmMapper::map(const void* input, void* output, size_t n_samples) {
    if (!input) {
        roc_panic("pcm mapper: input is null");
    }

    if (!output) {
        roc_panic("pcm mapper: output is null");
    }

    map_func_(input, output, n_samples);
}

} // namespace audio
} // namespace roc
