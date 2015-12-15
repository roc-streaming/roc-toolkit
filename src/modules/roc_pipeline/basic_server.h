/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/basic_server.h
//! @brief Base class for server pipeline.

#ifndef ROC_PIPELINE_BASIC_SERVER_H_
#define ROC_PIPELINE_BASIC_SERVER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/thread.h"
#include "roc_core/atomic.h"

#include "roc_datagram/idatagram.h"
#include "roc_datagram/idatagram_reader.h"

#include "roc_packet/ipacket_parser.h"

#include "roc_audio/isample_buffer_writer.h"
#include "roc_audio/isink.h"

#include "roc_pipeline/basic_session.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Base class for server pipeline.
//!
//! @remarks
//!  Fetches datagrams from input queue, manages sessions and
//!  generates audio.
//!
//! @see Server.
class BasicServer : public core::Thread, public core::NonCopyable<> {
public:
    //! Get number of active sessions.
    size_t num_sessions() const;

    //! Register port.
    //! @remarks
    //!  When datagram received with destination @p address, session will
    //!  use @p parser to create packet from datagram. If no port registered
    //!  for address, datagrams to that address will be dropped.
    void add_port(const datagram::Address& address, packet::IPacketParser& parser);

    //! Process input datagrams.
    //! @remarks
    //!  Fetches no more than @p n_datagrams from input datagram reader
    //!  and generates @p n_buffers sample buffers of @p n_samples samples.
    bool tick(size_t n_datagrams, size_t n_buffers, size_t n_samples);

    //! Stop thread.
    //! @remarks
    //!  May be called from any thread. After this call, subsequent join()
    //!  call will return as soon as current tick() returns.
    //! @note
    //!  This methods is not aware of pipeline components that may block
    //!  tick() and hence can not interrupt them.
    void stop();

protected:
    //! Initialize server.
    BasicServer(const ServerConfig& config);

    ~BasicServer();

    //! Run server thread.
    virtual void run();

    //! Create datagram reader.
    //! @remarks
    //!  Used to fetch input datagrams.
    virtual datagram::IDatagramReader* make_datagram_reader() = 0;

    //! Create audio sink.
    //! @remarks
    //!  Used to attach and detach sessions' renderers.
    virtual audio::ISink* make_audio_sink() = 0;

    //! Create audio reader.
    //! @remarks
    //!  Used to generate audio samples from renderers attached to sink.
    virtual audio::IStreamReader* make_audio_reader() = 0;

    //! Create audio writer.
    //! @remarks
    //!  Used to write output audio samples.
    virtual audio::ISampleBufferWriter* make_audio_writer() = 0;

    //! Get config.
    const ServerConfig& config() const;

    //! Destory all sessions.
    //! @remarks
    //!  Should be called in derived class destructor.
    void destroy_sessions();

private:
    enum { MaxPorts = ROC_CONFIG_MAX_PORTS };

    struct Port {
        datagram::Address address;
        packet::IPacketParser* parser;

        Port()
            : parser(NULL) {
        }
    };

    void make_pipeline_();

    void fetch_datagrams_(size_t n_datagrams);

    bool find_session_and_store_(const datagram::IDatagram&);
    bool create_session_and_store_(const datagram::IDatagram&);
    const Port* find_port_(const datagram::Address&);

    void update_sessions_();
    bool generate_audio_(size_t n_buffers, size_t n_samples);

    core::Array<Port, MaxPorts> ports_;
    core::List<BasicSession> sessions_;

    const ServerConfig config_;
    const size_t n_channels_;

    datagram::IDatagramReader* datagram_reader_;

    audio::ISink* audio_sink_;

    audio::IStreamReader* audio_reader_;
    audio::ISampleBufferWriter* audio_writer_;

    core::Atomic stop_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_BASIC_SERVER_H_
