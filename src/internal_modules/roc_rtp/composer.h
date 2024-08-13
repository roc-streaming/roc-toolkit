/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/composer.h
//! @brief RTP packet composer.

#ifndef ROC_RTP_COMPOSER_H_
#define ROC_RTP_COMPOSER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/icomposer.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

//! RTP packet composer.
class Composer : public packet::IComposer, public core::NonCopyable<> {
public:
    //! Initialization.
    //! @remarks
    //!  If @p inner_composer is not NULL, it is used to compose the packet payload.
    Composer(packet::IComposer* inner_composer, core::IArena& arena);

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

private:
    packet::IComposer* inner_composer_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_COMPOSER_H_
