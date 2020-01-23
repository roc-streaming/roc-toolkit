/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_port.h
//! @brief Sender port pipeline.

#ifndef ROC_PIPELINE_SENDER_PORT_H_
#define ROC_PIPELINE_SENDER_PORT_H_

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

//! Sender port pipeline.
//! @remarks
//!  Created at the sender side for every sending port.
class SenderPort : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    SenderPort(const PortConfig& config, core::IAllocator& allocator);

    //! Check if the port pipeline was succefully constructed.
    bool valid() const;

    //! Get protocol.
    address::EndpointProtocol proto() const;

    //! Get packet composer.
    packet::IComposer& composer();

    //! Set output writer.
    //! Called outside of roc_pipeline from any thread.
    void set_writer(packet::IWriter& writer);

    //! Check if writer is set.
    bool has_writer() const;

    //! Write packet.
    //! Called from pipeline thread.
    virtual void write(const packet::PacketPtr& packet);

private:
    core::Mutex mutex_;

    address::EndpointProtocol proto_;
    const address::SocketAddr dst_address_;

    packet::IWriter* writer_;
    packet::IComposer* composer_;

    core::ScopedPtr<rtp::Composer> rtp_composer_;
    core::ScopedPtr<packet::IComposer> fec_composer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_PORT_H_
