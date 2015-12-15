/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/server.h
//! @brief Server pipeline.

#ifndef ROC_PIPELINE_SERVER_H_
#define ROC_PIPELINE_SERVER_H_

#include "roc_core/maybe.h"

#include "roc_audio/channel_muxer.h"
#include "roc_audio/timed_writer.h"

#include "roc_pipeline/basic_server.h"

namespace roc {
namespace pipeline {

//! Server pipeline.
//!
//! Fetches datagrams from input queue, manages active sessions and their
//! storages and renderers, and generates audio stream.
//!
//! @b Queues
//!  - Input datagram queue is usually passed to network thread which writes
//!    incoming datagrams to it.
//!
//!  - Output sample buffer queue is usually passed to audio player thread
//!    which fetches samples from it and sends them to the sound card.
//!
//! @b Invocation
//!  - User may call start() to start server thread. The thread will call
//!    tick() in an infinite loop.
//!
//!  - Alternatively, user may periodically call tick().
//!
//! @b Customizing
//!  - User may provide custom ServerConfig with non-default options,
//!    channel mask, sizes, pools, etc.
//!
//!  - User may inherit BasicSession or Session and implement non-default
//!    session pipeline. To employ custom session implementation, user
//!    should set appropriate session pool in config, which acts as a factory.
//!
//!  - User may inherit BasicServer or Server and implement non-default
//!    server pipeline.
//!
//! @b Pipeline
//!
//!  Server pipeline consists of several steps:
//!
//!   <i> Fetching datagrams </i>
//!    - Fetch datagrams from input queue.
//!
//!    - Look at datagram's source address and check if a session exists for
//!      this address; if not, and parser exists for datagram's destination
//!      address, create new session using session pool.
//!
//!    - If new session was created, attach it to audio sink.
//!
//!    - If session existed or created, parse packet from datagram and store
//!      new packet into session.
//!
//!   <i> Updating state </i>
//!    - Update every session state.
//!
//!    - If session fails to update its state (probably bacause it detected
//!      that it's broken or inactive), session is unregistered from
//!      audio sink and removed.
//!
//!   <i> Generating samples </i>
//!    - Requests audio sink to generate samples. During this process,
//!      previously stored packets are transformed into audio stream.
//!
//! @see ServerConfig, BasicServer, BasicSession
class Server : public BasicServer {
public:
    //! Initialize server.
    //!
    //! @b Parameters
    //!  - @p datagram_reader specifies input datagram queue;
    //!  - @p audio_writer specifies output sample queue;
    //!  - @p config specifies server and session configuration.
    //!
    //! @note
    //!  If @p audio_writer blocks, tick() will also block when writing
    //!  output samples.
    Server(datagram::IDatagramReader& datagram_reader,
           audio::ISampleBufferWriter& audio_writer,
           const ServerConfig& config = ServerConfig());

    ~Server();

protected:
    //! Create datagram reader.
    virtual datagram::IDatagramReader* make_datagram_reader();

    //! Create audio sink.
    virtual audio::ISink* make_audio_sink();

    //! Create audio reader.
    virtual audio::IStreamReader* make_audio_reader();

    //! Create audio writer.
    virtual audio::ISampleBufferWriter* make_audio_writer();

    //! Input datagram reader.
    datagram::IDatagramReader& input_reader;

    //! Output audio writer.
    audio::ISampleBufferWriter& output_writer;

    //! Audio sink and audio reader.
    audio::ChannelMuxer channel_muxer;

    //! Constrains output speed.
    core::Maybe<audio::TimedWriter> timed_writer;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SERVER_H_
