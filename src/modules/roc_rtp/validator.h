/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/validator.h
//! @brief RTP validator.

#ifndef ROC_RTP_VALIDATOR_H_
#define ROC_RTP_VALIDATOR_H_

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
    //!  - @p sample_rate defines session sample rate
    Validator(packet::IReader& reader, const ValidatorConfig& config, size_t sample_rate);

    //! Read next packet.
    //! @remarks
    //!  Reads packet from the underlying reader and validates it. If the packet
    //!  is valid, return it. Otherwise, returns NULL.
    virtual packet::PacketPtr read();

private:
    bool check_(const packet::RTP& prev, const packet::RTP& next) const;

    packet::IReader& reader_;
    packet::PacketPtr prev_packet_;

    const ValidatorConfig config_;
    const size_t sample_rate_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_VALIDATOR_H_
