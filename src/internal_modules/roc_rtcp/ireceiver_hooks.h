/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/ireceiver_hooks.h
//! @brief Receiver hooks interface.

#ifndef ROC_RTCP_IRECEIVER_HOOKS_H_
#define ROC_RTCP_IRECEIVER_HOOKS_H_

#include "roc_packet/units.h"
#include "roc_rtcp/metrics.h"

namespace roc {
namespace rtcp {

//! Receiver hooks interface.
class IReceiverHooks {
public:
    virtual ~IReceiverHooks();

    //! Invoked when retrieved source description.
    virtual void on_update_source(packet::stream_source_t source_id,
                                  const char* cname) = 0;

    //! Invoked when retrieved source termination message.
    virtual void on_remove_source(packet::stream_source_t source_id) = 0;

    //! Get number of sources for which we send reception metrics.
    virtual size_t on_get_num_sources() = 0;

    //! Generate reception metrics for the source with given index.
    //! @p source_index is a number from 0 ro num_receipted_sources().
    virtual ReceptionMetrics on_get_reception_metrics(size_t source_index) = 0;

    //! Handle metrics obtained from sender.
    virtual void on_add_sending_metrics(const SendingMetrics& metrics) = 0;

    //! Handle estimated link metrics.
    virtual void on_add_link_metrics(const LinkMetrics& metrics) = 0;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_IRECEIVER_HOOKS_H_
