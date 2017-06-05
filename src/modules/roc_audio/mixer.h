/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/mixer.h
//! @brief Mixer.

#ifndef ROC_AUDIO_MIXER_H_
#define ROC_AUDIO_MIXER_H_

#include "roc_config/config.h"

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"

#include "roc_audio/istream_reader.h"
#include "roc_audio/sample_buffer.h"

namespace roc {
namespace audio {

//! Mixer.
//!
//! Mixes multiple input streams into one output stream.
//!
//! For example, these two input streams:
//! @code
//!  1, 2, 3, ...
//!  4, 5, 6, ...
//! @endcode
//!
//! are transformed into this output stream:
//! @code
//!  5, 7, 9, ...
//! @endcode
class Mixer : public IStreamReader, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit Mixer(ISampleBufferComposer& composer = default_buffer_composer());

    //! Read samples from output stream.
    virtual void read(const ISampleBufferSlice&);

    //! Add input stream.
    void add(IStreamReader&);

    //! Remove input stream.
    void remove(IStreamReader&);

private:
    core::List<IStreamReader, core::NoOwnership> readers_;

    ISampleBufferPtr temp_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_MIXER_H_
