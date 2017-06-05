/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/zipper.h
//! @brief Zipper.

#ifndef ROC_AUDIO_ZIPPER_H_
#define ROC_AUDIO_ZIPPER_H_

#include "roc_config/config.h"

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"

#include "roc_audio/istream_reader.h"
#include "roc_audio/sample_buffer.h"

namespace roc {
namespace audio {

//! Zipper.
//!
//! Combines multiple input streams into one output stream
//! in interleaved format.
//!
//! For example, these two input streams:
//! @code
//!  1, 3, 5, ...
//!  2, 4, 6, ...
//! @endcode
//!
//! are transformed into this output stream:
//! @code
//!  1, 2, 3, 4, 5, 6, ...
//! @endcode
class Zipper : public IStreamReader, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit Zipper(ISampleBufferComposer& composer = default_buffer_composer());

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

#endif // ROC_AUDIO_ZIPPER_H_
