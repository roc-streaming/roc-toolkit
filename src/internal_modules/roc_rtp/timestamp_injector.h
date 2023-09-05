/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/timestamp_injector.h
//! @brief Fills capture timestamp field in packets.

#ifndef ROC_RTP_TIMESTAMP_INJECTOR_H_
#define ROC_RTP_TIMESTAMP_INJECTOR_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace rtp {

//! Fills capture timestamps in rtp packets.
//!
//! Gets a pair of a reference unix-time stamp (in ns) and correspondant rtp timestamp,
//! and approximates this dependency to a passing packet.
class TimestampInjector : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    TimestampInjector(packet::IReader& packet_src, const audio::SampleSpec& sample_spec);

    //! Virtual destructor.
    virtual ~TimestampInjector();

    //! Get packet with filled capture ts field.
    //!
    //! If update_mapping has not been called yet, capture timestamp will be 0.
    virtual packet::PacketPtr read();

    //! Get a pair of a reference timestamps.
    void update_mapping(core::nanoseconds_t capture_ts, packet::timestamp_t rtp_ts);

private:
    bool has_ts_;
    core::nanoseconds_t ts_;
    packet::timestamp_t rtp_ts_;

    packet::IReader& reader_;
    const audio::SampleSpec sample_spec_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_TIMESTAMP_INJECTOR_H_
