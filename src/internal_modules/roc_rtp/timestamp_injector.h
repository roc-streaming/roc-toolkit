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
#include "roc_core/rate_limiter.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace rtp {

//! Fills capture timestamps in rtp packets.
//! @remarks
//!  Gets a pair of a reference unix-time stamp (in ns) and correspondent rtp timestamp,
//!  and approximates this dependency to a passing packet.
class TimestampInjector : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    TimestampInjector(packet::IReader& reader, const audio::SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get packet with filled capture ts field.
    //! @remarks
    //!  If update_mapping has not been called yet, capture timestamp will be 0.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& packet,
                                                       packet::PacketReadMode mode);

    //! Get a pair of a reference timestamps.
    void update_mapping(core::nanoseconds_t capture_ts,
                        packet::stream_timestamp_t rtp_ts);

private:
    bool has_ts_;
    core::nanoseconds_t capt_ts_;
    packet::stream_timestamp_t rtp_ts_;

    packet::IReader& reader_;
    const audio::SampleSpec sample_spec_;

    size_t n_drops_;

    core::RateLimiter rate_limiter_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_TIMESTAMP_INJECTOR_H_
