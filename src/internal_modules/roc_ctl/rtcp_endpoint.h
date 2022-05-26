/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/rtcp_endpoint.h
//! @brief RTCP endpoint.

#ifndef ROC_CTL_RTCP_ENDPOINT_H_
#define ROC_CTL_RTCP_ENDPOINT_H_

#include "roc_ctl/basic_control_endpoint.h"

namespace roc {
namespace ctl {

//! RTCP endpoint.
class RtcpEndpoint : public BasicControlEndpoint {
public:
    RtcpEndpoint(core::IAllocator&);

    //! Bind endpoint to local address.
    virtual bool bind(const address::EndpointUri& uri);

    //! Bind endpoint to remote address.
    virtual bool connect(const address::EndpointUri& uri);

    //! Close endpoint.
    virtual void close();

private:
    // TODO
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_RTCP_ENDPOINT_H_
