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
#include "roc_core/noncopyable.h"
#include "roc_core/unique_ptr.h"
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
    SenderPort(const PortConfig& config,
               packet::IWriter& writer,
               core::IAllocator& allocator);

    //! Check if the port pipeline was succefully constructed.
    bool valid() const;

    //! Get packet composer.
    packet::IComposer& composer();

    //! Write packet.
    void write(const packet::PacketPtr& packet);

private:
    const address::SocketAddr dst_address_;

    packet::IWriter& writer_;
    packet::IComposer* composer_;

    core::UniquePtr<rtp::Composer> rtp_composer_;
    core::UniquePtr<packet::IComposer> fec_composer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_PORT_H_
