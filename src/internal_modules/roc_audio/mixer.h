/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/mixer.h
//! @brief Mixer.

#ifndef ROC_AUDIO_MIXER_H_
#define ROC_AUDIO_MIXER_H_

#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Mixer.
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
class Mixer : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //! @p buffer_factory is used to allocate a temporary buffer for mixing.
    Mixer(core::BufferFactory<sample_t>& buffer_factory);

    //! Check if the mixer was succefully constructed.
    bool is_valid() const;

    //! Add input reader.
    void add_input(IFrameReader&);

    //! Remove input reader.
    void remove_input(IFrameReader&);

    //! Read audio frame.
    //! @remarks
    //!  Reads samples from every input reader, mixes them, and fills @p frame
    //!  with the result.
    virtual bool read(Frame& frame);

private:
    void read_(sample_t* out_data, size_t out_sz, unsigned& flags);

    core::List<IFrameReader, core::NoOwnership> readers_;
    core::Slice<sample_t> temp_buf_;

    bool valid_;
    const audio::SampleSpec samplespec_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_MIXER_H_
