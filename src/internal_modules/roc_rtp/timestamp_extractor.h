/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/timestamp_extractor.h
//! @brief Extracts capture timestamp field from packets.

#ifndef ROC_RTP_TIMESTAMP_EXTRACTOR_H_
#define ROC_RTP_TIMESTAMP_EXTRACTOR_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/stddefs.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace rtp {

//! Remembers a recent pair of capture timestamp and rtp ts.
class TimestampExtractor : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    TimestampExtractor(packet::IWriter& writer, const audio::SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Passes pkt downstream and remembers its capture and rtp timestamps.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& pkt);

    //! Check if mapping already available.
    bool has_mapping();

    //! Get rtp timestamp mapped to given capture timestamp.
    //! @pre
    //!  has_mapping() should return true, otherwise it will panic.
    packet::stream_timestamp_t get_mapping(core::nanoseconds_t capture_ts);

private:
    packet::IWriter& writer_;

    bool has_ts_;
    core::nanoseconds_t capt_ts_;
    packet::stream_timestamp_t rtp_ts_;

    const audio::SampleSpec sample_spec_;

    core::RateLimiter rate_limiter_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_TIMESTAMP_EXTRACTOR_H_
