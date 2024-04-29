/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/fanout.h
//! @brief Fanout.

#ifndef ROC_AUDIO_FANOUT_H_
#define ROC_AUDIO_FANOUT_H_

#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

//! Fanout.
//! Duplicates audio stream to multiple output writers.
class Fanout : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Check if writer is already added.
    bool has_output(IFrameWriter&);

    //! Add output writer.
    void add_output(IFrameWriter&);

    //! Remove output writer.
    void remove_output(IFrameWriter&);

    //! Write audio frame.
    //! @remarks
    //!  Writes samples to every output writer.
    virtual status::StatusCode write(Frame& frame);

private:
    core::List<IFrameWriter, core::NoOwnership> writers_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FANOUT_H_
