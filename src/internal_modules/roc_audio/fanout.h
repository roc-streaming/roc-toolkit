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

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Fanout.
//!
//! Duplicates audio stream to multiple output writers.
//!
//! Features:
//!  - Since StatusPart and StatusDrain are not allowed for write operations,
//!    fanout does not need any special handling for them.
//!
//!  - If pipeline element reports end-of-stream (StatusFinish), fanout skips this
//!    element until it's removed.
class Fanout : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    Fanout(const SampleSpec& sample_spec,
           FrameFactory& frame_factory,
           core::IArena& arena);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Check if writer is already added.
    bool has_output(IFrameWriter& writer);

    //! Add output writer.
    ROC_ATTR_NODISCARD status::StatusCode add_output(IFrameWriter& writer);

    //! Remove output writer.
    void remove_output(IFrameWriter& writer);

    //! Write audio frame.
    //! @remarks
    //!  Writes samples to every output writer.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

private:
    struct Output {
        // to where to write samples, typically sender session
        IFrameWriter* writer;
        // if true, output returned StatusFinish and should not be used
        bool is_finished;

        Output()
            : writer(NULL)
            , is_finished(false) {
        }
    };

    core::Array<Output, 8> outputs_;
    const SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FANOUT_H_
