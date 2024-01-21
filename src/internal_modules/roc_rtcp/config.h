/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/config.h
//! @brief RTCP config.

#ifndef ROC_RTCP_CONFIG_H_
#define ROC_RTCP_CONFIG_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_rtcp/rtt_estimator.h"

namespace roc {
namespace rtcp {

//! RTCP config.
struct Config {
    //! Timeout to remove inactive streams.
    core::nanoseconds_t inactivity_timeout;

    //! RTT estimation config.
    RttConfig rtt;

    //! Enable generation of SR/RR packets.
    bool enable_sr_rr;

    //! Enable generation of XR packets.
    bool enable_xr;

    //! Enable generation of SDES packets.
    bool enable_sdes;

    Config()
        : inactivity_timeout(core::Second * 5)
        , enable_sr_rr(true)
        , enable_xr(true)
        , enable_sdes(true) {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_CONFIG_H_
