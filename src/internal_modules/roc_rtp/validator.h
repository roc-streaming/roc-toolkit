/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/validator.h
//! @brief RTP validator.

#ifndef ROC_RTP_VALIDATOR_H_
#define ROC_RTP_VALIDATOR_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace rtp {

//! Validator parameters.
struct ValidatorConfig {
    //! Maximum allowed delta between two consecutive packet seqnums.
    size_t max_sn_jump;

    //! Maximum allowed delta between two consecutive packet timestamps, in nanoseconds.
    core::nanoseconds_t max_ts_jump;

    ValidatorConfig()
        : max_sn_jump(100)
        , max_ts_jump(core::Second) {
    }
};

//! RTP validator.
class Validator : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader
    //!  - @p config defines validator parameters
    //!  - @p sample_spec defines session sample spec
    Validator(packet::IReader& reader,
              const ValidatorConfig& config,
              const audio::SampleSpec& sample_spec);

    //! Read next packet.
    //!
    //! @remarks
    //!  Reads packet from the underlying reader and validates it.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& pp);

private:
    bool validate_(const packet::RTP& prev, const packet::RTP& next) const;

    packet::IReader& reader_;

    bool has_prev_packet_;
    packet::RTP prev_packet_rtp_;

    const ValidatorConfig config_;
    const audio::SampleSpec sample_spec_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_VALIDATOR_H_
