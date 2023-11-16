/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/print_supported.h
//! @brief Print supported encodings.

#ifndef ROC_AUDIO_PRINT_SUPPORTED_H_
#define ROC_AUDIO_PRINT_SUPPORTED_H_

#include "roc_core/attributes.h"

namespace roc {
namespace audio {

//! Print supported encodings.
ROC_ATTR_NODISCARD bool print_supported();

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PRINT_SUPPORTED_H_
