/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_format.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

const char* pcm_format_to_str(const PcmFormat& fmt) {
    return pcm_to_str(fmt.code, fmt.endian);
}

bool pcm_format_parse(const char* str, PcmFormat& fmt) {
    if (!str) {
        roc_panic("pcm: string is null");
    }
    return pcm_from_str(str, fmt.code, fmt.endian);
}

PcmTraits pcm_format_traits(const PcmFormat& fmt) {
    PcmTraits traits;

    traits.bit_depth = pcm_bit_depth(fmt.code);
    traits.bit_width = pcm_bit_width(fmt.code);
    traits.is_integer = pcm_is_integer(fmt.code);
    traits.is_signed = pcm_is_signed(fmt.code);

    return traits;
}

} // namespace audio
} // namespace roc
