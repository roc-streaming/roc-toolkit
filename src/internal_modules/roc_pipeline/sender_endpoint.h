/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_endpoint.h
//! @brief Sender endpoint pipeline.

#ifndef ROC_PIPELINE_SENDER_ENDPOINT_H_
#define ROC_PIPELINE_SENDER_ENDPOINT_H_

#include "roc_core/iallocator.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_pipeline/config.h"
#include "roc_rtcp/composer.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace pipeline {

//! Sender endpoint sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing packets for single network endpoint
class SenderEndpoint : public core::NonCopyable<>, private packet::IWriter {
public:
    //! Initialize.
    SenderEndpoint(address::Protocol proto, core::IAllocator& allocator);

    //! Check if pipeline was succefully constructed.
    bool valid() const;

    //! Get protocol.
    address::Protocol proto() const;

    //! Get packet composer.
    //! @remarks
    //!  This composer will creates packets according to endpoint protocol.
    packet::IComposer& composer();

    //! Get packet writer.
    //! @remarks
    //!  This writer will pass packets to the endpoint pipeline.
    packet::IWriter& writer();

    //! Check if destination writer was set.
    //! @remarks
    //!  True if set_destination_writer() was called.
    bool has_destination_writer() const;

    //! Set destination writer.
    //! @remarks
    //!  When packets are written to the endpoint pipeline, in the end they
    //!  go to the destination writer.
    void set_destination_writer(packet::IWriter& writer);

    //! Set destination address.
    //! @remarks
    //!  When packets are written to the endpoint pipeline, they are assigned
    //!  the specified destination address.
    void set_destination_address(const address::SocketAddr&);

private:
    virtual void write(const packet::PacketPtr& packet);

    const address::Protocol proto_;

    packet::IWriter* dst_writer_;
    address::SocketAddr dst_address_;

    packet::IComposer* composer_;

    core::Optional<rtp::Composer> rtp_composer_;
    core::ScopedPtr<packet::IComposer> fec_composer_;
    core::Optional<rtcp::Composer> rtcp_composer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_ENDPOINT_H_
