/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/fanout.h
//! @brief Fanout.

#ifndef ROC_AUDIO_FANOUT_H_
#define ROC_AUDIO_FANOUT_H_

#include "roc_audio/iwriter.h"
#include "roc_audio/units.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/pool.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

//! Fanout.
//! Duplicates audio stream to multiple output writers.
class Fanout : public IWriter, public core::NonCopyable<> {
public:
    //! Check if writer is already added.
    bool has_output(IWriter&);

    //! Add output writer.
    void add_output(IWriter&);

    //! Remove output writer.
    void remove_output(IWriter&);

    //! Write audio frame.
    //! @remarks
    //!  Writes samples to every output writer.
    virtual void write(Frame& frame);

private:
    core::List<IWriter, core::NoOwnership> writers_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FANOUT_H_
