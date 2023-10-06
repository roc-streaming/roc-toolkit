/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/populator.h
//! @brief RTP populator.

#ifndef ROC_RTP_POPULATOR_H_
#define ROC_RTP_POPULATOR_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace rtp {

//! RTP populator.
class Populator : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    Populator(packet::IReader& reader,
              audio::IFrameDecoder& decoder,
              const audio::SampleSpec& sample_spec);

    //! Read next packet.
    virtual status::StatusCode read(packet::PacketPtr&);

private:
    packet::IReader& reader_;
    audio::IFrameDecoder& decoder_;
    const audio::SampleSpec sample_spec_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_POPULATOR_H_
