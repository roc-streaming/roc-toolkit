/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/format.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const FormatTraits formats[] = {
    { Format_Pcm, "pcm",
      Format_SupportsNetwork | Format_SupportsDevices | Format_SupportsFiles },
    { Format_Wav, "wav", Format_SupportsFiles },
};

} // namespace

FormatTraits format_traits(Format format) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(formats); n++) {
        if (formats[n].id == format) {
            return formats[n];
        }
    }

    FormatTraits ret;
    memset(&ret, 0, sizeof(ret));
    ret.id = Format_Invalid;
    return ret;
}

const char* format_to_str(Format format) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(formats); n++) {
        if (formats[n].id == format) {
            return formats[n].name;
        }
    }

    return "invalid";
}

Format format_from_str(const char* str) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(formats); n++) {
        if (strcmp(formats[n].name, str) == 0) {
            return formats[n].id;
        }
    }

    return Format_Invalid;
}

} // namespace audio
} // namespace roc
