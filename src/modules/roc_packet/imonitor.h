/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/imonitor.h
//! @brief Monitor interface.

#ifndef ROC_PACKET_IMONITOR_H_
#define ROC_PACKET_IMONITOR_H_

#include "roc_core/list_node.h"

namespace roc {
namespace packet {

//! Monitor interface.
class IMonitor : public core::ListNode {
public:
    virtual ~IMonitor();

    //! Update session.
    //! @returns
    //!  false if session is broken and should be destroyed.
    virtual bool update() = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IMONITOR_H_
