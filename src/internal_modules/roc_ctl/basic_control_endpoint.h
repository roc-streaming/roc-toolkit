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

#include "roc_address/network_uri.h"
#include "roc_core/list_node.h"
#include "roc_core/ref_counted.h"
#include "roc_ctl/control_task.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_pipeline/sender_loop.h"

namespace roc {
namespace ctl {

//! Base class for control endpoints.
class BasicControlEndpoint
    : public core::RefCounted<BasicControlEndpoint, core::ArenaAllocation>,
      public core::ListNode<> {
public:
    //! Initialization.
    BasicControlEndpoint(core::IArena&);

    virtual ~BasicControlEndpoint();

    //! Check if endpoint is successfully bound to local URI.
    virtual bool is_bound() const = 0;

    //! Check if endpoint is successfully connected to remote URI.
    virtual bool is_connected() const = 0;

    //! Initiate asynchronous binding to local URI.
    //! On completion, resumes @p notify_task.
    virtual bool async_bind(const address::NetworkUri& uri, ControlTask& notify_task) = 0;

    //! Initiate asynchronous connecting to remote URI.
    //! Should be called after successfull bind.
    //! On completion, resumes @p notify_task.
    virtual bool async_connect(const address::NetworkUri& uri,
                               ControlTask& notify_task) = 0;

    //! Initiate asynchronous closing of endpoint.
    //! On completion, resumes @p notify_task.
    virtual void async_close(ControlTask& notify_task) = 0;

    //! Add sink pipeline controlled by this endpoint.
    //! Should be called after successfull bind.
    virtual bool attach_sink(const address::NetworkUri& uri,
                             pipeline::SenderLoop& sink) = 0;

    //! Remove sink pipeline.
    //! Should be called for earlier attached sink.
    virtual bool detach_sink(pipeline::SenderLoop& sink) = 0;

    //! Add source pipeline controlled by this endpoint.
    //! Should be called after successfull bind.
    virtual bool attach_source(const address::NetworkUri& uri,
                               pipeline::ReceiverLoop& source) = 0;

    //! Remove source pipeline.
    //! Should be called for earlier attached source.
    virtual bool detach_source(pipeline::ReceiverLoop& source) = 0;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_BASIC_CONTROL_ENDPOINT_H_
