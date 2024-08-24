/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

PcmMapper::PcmMapper(PcmSubformat input_fmt, PcmSubformat output_fmt)
    : input_fmt_(input_fmt)
    , output_fmt_(output_fmt)
    , input_traits_(pcm_subformat_traits(input_fmt))
    , output_traits_(pcm_subformat_traits(output_fmt)) {
    // To reduce code size, we generate converters only between raw and non-raw formats.
    // To convert between two non-raw formats, you need a pair of pcm mappers.
    roc_panic_if_msg(input_fmt != PcmSubformat_Raw && output_fmt != PcmSubformat_Raw,
                     "pcm mapper: either input or output format must be raw");

    // This must not happen if checks above passed.
    if (!(map_func_ = pcm_subformat_mapfn(input_fmt, output_fmt))) {
        roc_panic("pcm mapper: unable to select mapping function");
    }
}

PcmSubformat PcmMapper::input_format() const {
    return input_fmt_;
}

PcmSubformat PcmMapper::output_format() const {
    return output_fmt_;
}

size_t PcmMapper::input_sample_count(size_t input_bytes) const {
    return input_bytes * 8 / input_traits_.bit_width;
}

size_t PcmMapper::output_sample_count(size_t output_bytes) const {
    return output_bytes * 8 / output_traits_.bit_width;
}

size_t PcmMapper::input_byte_count(size_t input_samples) const {
    return (input_samples * input_traits_.bit_width + 7) / 8;
}

size_t PcmMapper::output_byte_count(size_t output_samples) const {
    return (output_samples * output_traits_.bit_width + 7) / 8;
}

size_t PcmMapper::input_bit_count(size_t input_samples) const {
    return input_samples * input_traits_.bit_width;
}

size_t PcmMapper::output_bit_count(size_t output_samples) const {
    return output_samples * output_traits_.bit_width;
}

size_t PcmMapper::map(const void* in_data,
                      size_t in_byte_size,
                      size_t& in_bit_off,
                      void* out_data,
                      size_t out_byte_size,
                      size_t& out_bit_off,
                      size_t n_samples) {
    roc_panic_if_msg(!in_data, "pcm mapper: input is null");
    roc_panic_if_msg(!out_data, "pcm mapper: output is null");

    roc_panic_if_msg(in_bit_off > in_byte_size * 8,
                     "pcm mapper: input offset out of bounds");
    roc_panic_if_msg(out_bit_off > out_byte_size * 8,
                     "pcm mapper: output offset out of bounds");

    n_samples =
        std::min(n_samples, (in_byte_size * 8 - in_bit_off) / input_traits_.bit_width);
    n_samples =
        std::min(n_samples, (out_byte_size * 8 - out_bit_off) / output_traits_.bit_width);

    if (n_samples != 0) {
        map_func_((const uint8_t*)in_data, in_bit_off, (uint8_t*)out_data, out_bit_off,
                  n_samples);
    }

    return n_samples;
}

} // namespace audio
} // namespace roc
