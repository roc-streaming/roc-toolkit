/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/parser.h
//! @brief RTP packet parser.

#ifndef ROC_RTP_PARSER_H_
#define ROC_RTP_PARSER_H_

#include "roc_core/heap_pool.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ipacket_parser.h"
#include "roc_rtp/audio_packet.h"
#include "roc_rtp/container_packet.h"

namespace roc {
namespace rtp {

//! RTP packet parser.
class Parser : public packet::IPacketParser, public core::NonCopyable<> {
public:
    //! Initialize.
    Parser(core::IPool<AudioPacket>& audio_pool = core::HeapPool<AudioPacket>::instance(),
           core::IPool<ContainerPacket>& container_pool =
               core::HeapPool<ContainerPacket>::instance());

    //! Parse packet.
    virtual packet::IPacketConstPtr parse(const core::IByteBufferConstSlice& buffer);

private:
    core::IPool<AudioPacket>& audio_pool_;
    core::IPool<ContainerPacket>& container_pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PARSER_H_
