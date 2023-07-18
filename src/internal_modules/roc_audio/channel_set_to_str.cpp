/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_set_to_str.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace audio {

channel_set_to_str::channel_set_to_str(const ChannelSet& ch_set) {
    core::StringBuilder bld(buf_, sizeof(buf_));

    format_channel_set(ch_set, bld);
}

} // namespace audio
} // namespace roc
