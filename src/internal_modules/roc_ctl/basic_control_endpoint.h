/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/basic_control_endpoint.h
//! @brief Base class for control endpoints.

#ifndef ROC_CTL_BASIC_CONTROL_ENDPOINT_H_
#define ROC_CTL_BASIC_CONTROL_ENDPOINT_H_

#include "roc_address/endpoint_uri.h"
#include "roc_core/list_node.h"
#include "roc_core/ref_counted.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/sender_sink.h"

namespace roc {
namespace ctl {

//! Base class for control endpoints.
class BasicControlEndpoint
    : public core::RefCounted<BasicControlEndpoint, core::StandardAllocation>,
      public core::ListNode {
public:
    BasicControlEndpoint(core::IAllocator&);

    virtual ~BasicControlEndpoint();

    //! Bind endpoint to local address.
    virtual bool bind(const address::EndpointUri& uri) = 0;

    //! Bind endpoint to remote address.
    virtual bool connect(const address::EndpointUri& uri) = 0;

    //! Close endpoint.
    virtual void close() = 0;

#if 0
    //! Attach sink pipeline to endpoint.
    virtual bool attach_sink(const address::EndpointUri& uri,
                             pipeline::SenderSink& sink) = 0;

    //! Detach sink pipeline from endpoint.
    virtual bool detach_sink(pipeline::SenderSink& sink) = 0;

    //! Attach source pipeline to endpoint.
    virtual bool attach_source(const address::EndpointUri& uri,
                               pipeline::ReceiverSource& source) = 0;

    //! Detach source pipeline from endpoint.
    virtual bool detach_source(pipeline::ReceiverSource& source) = 0;
#endif
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_BASIC_CONTROL_ENDPOINT_H_
