/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/composer.h
//! @brief RTCP packet composer.

#ifndef ROC_RTCP_COMPOSER_H_
#define ROC_RTCP_COMPOSER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/icomposer.h"

namespace roc {
namespace rtcp {

//! RTCP packet composer.
//!
//! @remarks
//!  Unlike other composers, this one expects that the buffer already contains valid
//!  RTCP compound packet. The actual composing is done earlier in rtcp::Communicator
//!  using rtcp::Builder.
class Composer : public packet::IComposer, public core::NonCopyable<> {
public:
    //! Initialization.
    explicit Composer(core::IArena& arena);

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Adjust buffer to align payload.
    virtual bool
    align(core::Slice<uint8_t>& buffer, size_t header_size, size_t payload_alignment);

    //! Prepare buffer for composing a packet.
    virtual bool
    prepare(packet::Packet& packet, core::Slice<uint8_t>& buffer, size_t payload_size);

    //! Pad packet.
    virtual bool pad(packet::Packet& packet, size_t padding_size);

    //! Compose packet to buffer.
    virtual bool compose(packet::Packet& packet);
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_COMPOSER_H_
