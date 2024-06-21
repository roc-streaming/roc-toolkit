/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/filter.h
//! @brief RTP filter.

#ifndef ROC_RTP_FILTER_H_
#define ROC_RTP_FILTER_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace rtp {

//! RTP filter parameters.
struct FilterConfig {
    //! Maximum allowed delta between two consecutive packet seqnums.
    //! If exceeded, packet is dropped.
    size_t max_sn_jump;

    //! Maximum allowed delta between two consecutive packet timestamps, in nanoseconds.
    //! If exceeded, packet is dropped.
    core::nanoseconds_t max_ts_jump;

    FilterConfig()
        : max_sn_jump(100)
        , max_ts_jump(core::Second) {
    }
};

//! RTP filter.
//!
//! Performs initial validation and initialization of incoming sequence
//! of RTP packets.
//!
//!  - Validates sequence of incoming RTP packets and detects disturbances,
//!    like seqnum jumps, timestamp jumps, SSRC changes, etc.
//!
//!  - Populates local fields (that are not carried over network),
//!    currently packet duration (based on provided payload decoder).
class Filter : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is used to read input packets
    //!  - @p decoder is used to query parameters of packets
    //!  - @p config defines filtering parameters
    //!  - @p sample_spec defines stream sample spec
    Filter(packet::IReader& reader,
           audio::IFrameDecoder& decoder,
           const FilterConfig& config,
           const audio::SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read next packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& pp,
                                                       packet::PacketReadMode mode);

private:
    bool validate_(const packet::PacketPtr& packet);
    void populate_(const packet::PacketPtr& packet);

    bool validate_sequence_(const packet::RTP& prev, const packet::RTP& next) const;

    packet::IReader& reader_;
    audio::IFrameDecoder& decoder_;

    bool has_prev_packet_;
    packet::RTP prev_packet_rtp_;

    const FilterConfig config_;
    const audio::SampleSpec sample_spec_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FILTER_H_
