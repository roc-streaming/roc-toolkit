/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/client.h
//! @brief Client pipeline.

#ifndef ROC_PIPELINE_CLIENT_H_
#define ROC_PIPELINE_CLIENT_H_

#include "roc_core/noncopyable.h"
#include "roc_core/maybe.h"
#include "roc_core/thread.h"

#include "roc_datagram/idatagram_composer.h"
#include "roc_datagram/idatagram_writer.h"

#include "roc_packet/ipacket_composer.h"
#include "roc_packet/packet_sender.h"
#include "roc_packet/spoiler.h"
#include "roc_packet/interleaver.h"

#include "roc_fec/encoder.h"

#ifdef ROC_TARGET_OPENFEC
#include "roc_fec/of_block_encoder.h"
#endif

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"
#include "roc_audio/splitter.h"
#include "roc_audio/timed_writer.h"

#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Client pipeline.
//!
//! Fetches samples from input queue, composes datagrams, and writes them
//! to output queue.
//!
//! @b Queues
//!  - Input sample buffer queue is usually passed to audio grabber thread
//!    which writes grabbed audio to the queue.
//!
//!  - Output datagram queue is usually passed to network thread, which
//!    which fetches datagrams from the queue and sends them to remote host.
//!
//! @b Invocation
//!  - User may call start() to start client thread. The thread will call
//!    tick() in an infinite loop.
//!
//!  - Alternatively, user may periodically call tick().
//!
//! @b Pipeline
//!
//!   Client pipeline consists of several steps:
//!
//!   <i> Fetching samples </i>
//!   - Fetch sample buffers from input queue.
//!
//!   <i> Generating packets </i>
//!   - Split fetched sample buffers into fixed-size audio packets.
//!
//!   - Process produced packet sequence. Processing may include
//!     FEC encoding and reordering.
//!
//!   <i> Generating datagrams </i>
//!   - Generate datagram for every packet and add it to output queue.
//!
//! @see ClientConfig
class Client : public core::Thread, public core::NonCopyable<> {
public:
    //! Initialize client.
    //!
    //! @b Parameters
    //!  - @p audio_reader specifies input sample queue;
    //!  - @p datagram_writer specifies output datagram queue;
    //!  - @p datagram_composer is used to construc output datagrams;
    //!  - @p packet_composer is used to construc output packets;
    //!  - @p config specifies client configuration.
    Client(audio::ISampleBufferReader& audio_reader,
           datagram::IDatagramWriter& datagram_writer,
           datagram::IDatagramComposer& datagram_composer,
           packet::IPacketComposer& packet_composer,
           const ClientConfig& config = ClientConfig());

    //! Set datagram sender address.
    void set_sender(const datagram::Address&);

    //! Set datagram receiver address.
    void set_receiver(const datagram::Address&);

    //! Process input samples.
    //! @remarks
    //!  Fetches one sample buffer from input reader.
    bool tick();

    //! Flush buffered samples and packets.
    void flush();

private:
    virtual void run();

    audio::ISampleBufferWriter* make_audio_writer_();

    packet::IPacketWriter* make_packet_writer_();
    packet::IPacketWriter* make_fec_encoder_(packet::IPacketWriter*);

    const ClientConfig config_;

    packet::PacketSender packet_sender_;
    packet::IPacketComposer& packet_composer_;

    core::Maybe<packet::Spoiler> spoiler_;
    core::Maybe<packet::Interleaver> interleaver_;

#ifdef ROC_TARGET_OPENFEC
    core::Maybe<fec::OF_BlockEncoder> fec_ldpc_encoder_;
    core::Maybe<fec::Encoder> fec_encoder_;
#endif

    core::Maybe<audio::Splitter> splitter_;
    core::Maybe<audio::TimedWriter> timed_writer_;

    audio::ISampleBufferReader& audio_reader_;
    audio::ISampleBufferWriter& audio_writer_;

    datagram::IDatagramWriter& datagram_writer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CLIENT_H_
