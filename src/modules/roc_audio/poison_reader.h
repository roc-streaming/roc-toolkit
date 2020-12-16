/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/poison_reader.h
//! @brief Poison reader.

#ifndef ROC_AUDIO_POISON_READER_H_
#define ROC_AUDIO_POISON_READER_H_

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Poisons audio frames before reading them.
class PoisonReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    PoisonReader(IReader& reader);

    //! Read audio frame.
    virtual ssize_t read(Frame&);

private:
    IReader& reader_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_POISON_READER_H_
