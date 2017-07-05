/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
#include "roc_packet/ireader.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace rtp {

//! Validator parameters.
struct ValidatorConfig {
    //! Maximum allowed delta between two consecutive packet seqnums.
    size_t max_sn_jump;

    //! Maximum allowed delta between two consecutive packet timestamps, in milliseconds.
    size_t max_ts_jump;

    ValidatorConfig()
        : max_sn_jump(100)
        , max_ts_jump(1000) {
    }
};

//! RTP validator.
class Validator : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader
    //!  - @p format defines the expected packet payload type
    //!  - @p config defines validator parameters
    Validator(packet::IReader& reader,
              const Format& format,
              const ValidatorConfig& config);

    //! Read next packet.
    //! @remarks
    //!  Reads packet from the underlying reader and validates it. If the packet
    //!  is valid, return it. Otherwise, returns NULL.
    virtual packet::PacketPtr read();

private:
    bool check_(const packet::RTP& prev, const packet::RTP& next) const;

    const Format& format_;

    packet::IReader& reader_;
    packet::PacketPtr prev_packet_;

    const ValidatorConfig config_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_VALIDATOR_H_
