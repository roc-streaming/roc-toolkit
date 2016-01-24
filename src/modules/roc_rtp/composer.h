/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_core/ipool.h"
#include "roc_core/heap_pool.h"
#include "roc_datagram/default_buffer_composer.h"
#include "roc_packet/ipacket_composer.h"
#include "roc_rtp/audio_packet.h"
#include "roc_rtp/fec_packet.h"

namespace roc {
namespace rtp {

//! RTP packet composer.
class Composer : public packet::IPacketComposer, public core::NonCopyable<> {
public:
    //! Initialize.
    Composer(
        core::IPool<AudioPacket>& audio_pool = core::HeapPool<AudioPacket>::instance(),
        core::IPool<FECPacket>& fec_pool = core::HeapPool<FECPacket>::instance(),
        core::IByteBufferComposer& buffer_composer = datagram::default_buffer_composer());

    //! Compose packet.
    virtual packet::IPacketPtr compose(int options);

private:
    core::IPool<AudioPacket>& audio_pool_;
    core::IPool<FECPacket>& fec_pool_;

    core::IByteBufferComposer& buffer_composer_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_COMPOSER_H_
