/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_format.h"

namespace roc {
namespace audio {

const char* sample_format_to_str(SampleFormat format) {
    switch (format) {
    case SampleFormat_Pcm:
        return "pcm";

    case SampleFormat_Invalid:
        break;
    }

    return "invalid";
}

} // namespace audio
} // namespace roc
