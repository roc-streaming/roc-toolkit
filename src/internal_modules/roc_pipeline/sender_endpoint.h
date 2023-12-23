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

#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/shipper.h"
#include "roc_pipeline/config.h"
#include "roc_rtcp/composer.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace pipeline {

//! Sender endpoint sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing packets for single network endpoint
class SenderEndpoint : public core::NonCopyable<> {
public:
    //! Initialize.
    //!  - @p dest_address specifies destination address that is assigned to the
    //!    outgoing packets in the end of endpoint pipeline
    //!  - @p dest_writer specifies destination writer to which packets are sent
    //!    in the end of endpoint pipeline
    SenderEndpoint(address::Protocol proto,
                   const address::SocketAddr& dest_address,
                   packet::IWriter& dest_writer,
                   core::IArena& arena);

    //! Check if pipeline was succefully constructed.
    bool is_valid() const;

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

private:
    const address::Protocol proto_;

    packet::IComposer* composer_;

    core::Optional<rtp::Composer> rtp_composer_;
    core::ScopedPtr<packet::IComposer> fec_composer_;
    core::Optional<rtcp::Composer> rtcp_composer_;
    core::Optional<packet::Shipper> packet_shipper_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_ENDPOINT_H_
