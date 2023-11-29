/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"

namespace roc {
namespace audio {

void format_sample_spec(const SampleSpec& sample_spec, core::StringBuilder& bld) {
    bld.append_str("<sspec rate=");
    bld.append_uint(sample_spec.sample_rate(), 10);
    bld.append_str(" chset=");
    format_channel_set(sample_spec.channel_set(), bld);
    bld.append_str(">");
}

} // namespace audio
} // namespace roc
