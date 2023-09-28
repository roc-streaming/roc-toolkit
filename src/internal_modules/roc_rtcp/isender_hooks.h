/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/isender_hooks.h
//! @brief Sender hooks interface.

#ifndef ROC_RTCP_ISENDER_HOOKS_H_
#define ROC_RTCP_ISENDER_HOOKS_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_rtcp/metrics.h"

namespace roc {
namespace rtcp {

//! Sender hooks interface.
class ISenderHooks {
public:
    virtual ~ISenderHooks();

    //! Get number of sources produced by sender.
    virtual size_t on_get_num_sources() = 0;

    //! Get identifier of the source with given index.
    //! @p source_index is a number from 0 ro num_receipted_sources().
    virtual packet::stream_source_t on_get_sending_source(size_t source_index) = 0;

    //! Generate sending metrics.
    //! The obtained metrics will be sent to receiver(s).
    //! @remarks
    //!  @p report_time defines time point relative to which metrics should be
    //!  calculated, measured in nanoseconds since Unix epoch.
    virtual SendingMetrics on_get_sending_metrics(core::nanoseconds_t report_time) = 0;

    //! Handle reception feedback metrics obtained from receiver.
    //! Called for each source.
    virtual void on_add_reception_metrics(const ReceptionMetrics& metrics) = 0;

    //! Handle estimated link metrics.
    virtual void on_add_link_metrics(const LinkMetrics& metrics) = 0;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_ISENDER_HOOKS_H_
