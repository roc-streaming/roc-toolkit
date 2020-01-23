/*
 * Copyright (c) 2017 Roc authors
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
#include "roc_core/scoped_ptr.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace pipeline {

//! Sender endpoint pipeline.
//! @remarks
//!  Created for every transport endpoint. Belongs to endpoint set.
//!  Passes packets to outside writer (e.g. netio).
class SenderEndpoint : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    SenderEndpoint(address::EndpointProtocol proto, core::IAllocator& allocator);

    //! Check if pipeline was succefully constructed.
    bool valid() const;

    //! Get protocol.
    address::EndpointProtocol proto() const;

    //! Get packet composer.
    packet::IComposer& composer();

    //! Check if writer is set.
    bool has_writer() const;

    //! Set output writer.
    //! @remarks
    //!  Called outside of roc_pipeline from any thread.
    void set_output_writer(packet::IWriter& writer);

    //! Set destination UDP address.
    //! @remarks
    //!  Called outside of roc_pipeline from any thread.
    void set_destination_udp_address(const address::SocketAddr&);

    //! Write packet.
    //! @remarks
    //!  Called from pipeline thread.
    virtual void write(const packet::PacketPtr& packet);

private:
    core::Mutex mutex_;

    const address::EndpointProtocol proto_;
    address::SocketAddr udp_address_;

    packet::IWriter* writer_;
    packet::IComposer* composer_;

    core::ScopedPtr<rtp::Composer> rtp_composer_;
    core::ScopedPtr<packet::IComposer> fec_composer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_ENDPOINT_H_
