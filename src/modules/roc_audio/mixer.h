/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/mixer.h
//! @brief Mixer.

#ifndef ROC_AUDIO_MIXER_H_
#define ROC_AUDIO_MIXER_H_

#include "roc_audio/ireader.h"
#include "roc_audio/units.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/pool.h"
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
class Mixer : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p pool is used to allocate a temporary buffer of samples
    //!  - @p frame_length defines the temporary buffer length used to
    //!    read from, in nanoseconds
    //!  - @p sample_rate defines the number of samples taken from the audio signal
    //!  - @p ch_mask defines the bitmask of audio channels
    //!    attached readers
    explicit Mixer(core::BufferPool<sample_t>& pool,
                   core::nanoseconds_t frame_length,
                   size_t sample_rate,
                   packet::channel_mask_t ch_mask);

    //! Check if the mixer was succefully constructed.
    bool valid() const;

    //! Add input reader.
    void add_input(IReader&);

    //! Remove input reader.
    void remove_input(IReader&);

    //! Read audio frame.
    //! @remarks
    //!  Reads samples from every input reader, mixes them, and fills @p frame
    //!  with the result.
    virtual ssize_t read(Frame& frame);

private:
    void read_(sample_t* out_data, size_t out_sz);

    core::List<IReader, core::NoOwnership> readers_;
    core::Slice<sample_t> temp_buf_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_MIXER_H_
