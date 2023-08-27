/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/ts_filler.h
//! @brief Fills capture timestamp field in packets.

#ifndef ROC_TOOLKIT_CAPTURE_TS_FILLER_H
#define ROC_TOOLKIT_CAPTURE_TS_FILLER_H

#include "roc_packet/ireader.h"
#include "roc_core/noncopyable.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace rtp {

class CaptureTsFiller : public packet::IReader, public core::NonCopyable<> {
public:
    CaptureTsFiller(packet::IReader& packet_src, const audio::SampleSpec& sample_spec);
    virtual ~CaptureTsFiller();

    virtual packet::PacketPtr read();

    void set_current_timestamp(core::nanoseconds_t capture_ts,
                               packet::timestamp_t rtp_ts);

private:
    bool valid_ts_;
    core::nanoseconds_t ts_;
    packet::timestamp_t rtp_ts_;

    packet::IReader &reader_;
    const audio::SampleSpec sample_spec_;

};

} // namespace rtp
} // namespace roc


#endif // ROC_TOOLKIT_CAPTURE_TS_FILLER_H
