/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/loss_estimator.h
//! @brief Loss estimator.

#ifndef ROC_RTCP_LOSS_ESTIMATOR_H_
#define ROC_RTCP_LOSS_ESTIMATOR_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace rtcp {

//! Computes fractions loss ration since last report.
class LossEstimator {
public:
    //! Initialize.
    LossEstimator();

    //! Update and return fractional loss ration since previous update.
    //! @p expected_packets defines total count of packets expected.
    //! @p lost_packets defines count of packets not received,
    //! probably negative dues to duplicates.
    float update(uint64_t total_packets, int64_t lost_packets);

private:
    uint64_t prev_total_;
    int64_t prev_lost_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_LOSS_ESTIMATOR_H_
