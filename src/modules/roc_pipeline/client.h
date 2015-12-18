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

#include "roc_config/config.h"
#include "roc_core/maybe.h"

#include "roc_datagram/idatagram_composer.h"
#include "roc_datagram/idatagram_writer.h"

#include "roc_packet/ipacket_composer.h"
#include "roc_packet/packet_sender.h"
#include "roc_packet/wrecker.h"
#include "roc_packet/interleaver.h"

#include "roc_fec/encoder.h"

#ifdef ROC_TARGET_OPENFEC
#include "roc_fec/ldpc_block_encoder.h"
#endif

#include "roc_audio/splitter.h"
#include "roc_audio/timed_writer.h"

#include "roc_pipeline/basic_client.h"

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
//! @b Customizing
//!  - User may provide custom ClientConfig with non-default options, channel
//!    mask, sizes, pools, etc.
//!
//!  - User may inherit BasicClient or Client and implement non-default
//!    pipeline construction.
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
//! @see ClientConfig, BasicClient
class Client : public BasicClient {
public:
    //! Initialize client.
    //!
    //! @b Parameters
    //!  - @p audio_reader specifies input sample queue;
    //!  - @p datagram_writer specifies output datagram queue;
    //!  - @p config specifies client configuration.
    //!
    //! @note
    //!  If @p audio_reader blocks, tick() will also block when reading
    //!  input samples.
    Client(audio::ISampleBufferReader& audio_reader,
           datagram::IDatagramWriter& datagram_writer,
           const ClientConfig& config = ClientConfig());

    //! Set packet and datagram composers.
    virtual void set_composers(packet::IPacketComposer&, datagram::IDatagramComposer&);

    //! Set datagram sender address.
    virtual void set_sender(const datagram::Address&);

    //! Set datagram receiver address.
    virtual void set_receiver(const datagram::Address&);

protected:
    //! Create input audio reader.
    virtual audio::ISampleBufferReader* make_audio_reader();

    //! Create output audio reader.
    //! @remarks
    //!  Calls make_packet_writer().
    virtual audio::ISampleBufferWriter* make_audio_writer();

    //! Create output packet writer.
    //! @remarks
    //!  Calls make_fec_writer() if EnableLDPC option is set.
    virtual packet::IPacketWriter* make_packet_writer();

    //! Create FEC encoder.
    packet::IPacketWriter* make_fec_encoder(packet::IPacketWriter*);

    //! Reads input samples.
    audio::ISampleBufferReader& input_reader;

    //! Sends outgoing packets to output datagram queue.
    packet::PacketSender packet_sender;

    //! Creates outgoing packets.
    packet::IPacketComposer* packet_composer;

    //! Wrecks output packets.
    core::Maybe<packet::Wrecker> wrecker;

    //! Reorders outgoing packets.
    core::Maybe<packet::Interleaver> interleaver;

#ifdef ROC_TARGET_OPENFEC
    //! FEC codec implementation.
    core::Maybe<fec::LDPC_BlockEncoder> fec_ldpc_encoder;

    //! Encodes FEC packets.
    core::Maybe<fec::Encoder> fec_encoder;
#endif

    //! Splits audio stream into packets.
    core::Maybe<audio::Splitter> splitter;

    //! Constrains processing speed.
    core::Maybe<audio::TimedWriter> timed_writer;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CLIENT_H_
