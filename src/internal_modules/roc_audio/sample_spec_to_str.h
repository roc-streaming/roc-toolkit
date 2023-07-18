/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_spec_to_str.h
//! @brief Format SampleSpec to string.

#ifndef ROC_AUDIO_SAMPLE_SPEC_TO_STR_H_
#define ROC_AUDIO_SAMPLE_SPEC_TO_STR_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Format SampleSpec to string.
class sample_spec_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit sample_spec_to_str(const SampleSpec&);

    //! Get formatted string.
    const char* c_str() const {
        return buf_;
    }

private:
    char buf_[192];
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_SPEC_TO_STR_H_
