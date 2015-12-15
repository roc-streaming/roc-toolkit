/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/server.h"

namespace roc {
namespace pipeline {

Server::Server(datagram::IDatagramReader& datagram_reader,
               audio::ISampleBufferWriter& audio_writer,
               const ServerConfig& cfg)
    : BasicServer(cfg)
    , input_reader(datagram_reader)
    , output_writer(audio_writer)
    , channel_muxer(config().channels, *config().sample_buffer_composer) {
}

Server::~Server() {
    /* Destroy sessions before destroying sink and other objects.
     */
    destroy_sessions();
}

datagram::IDatagramReader* Server::make_datagram_reader() {
    return &input_reader;
}

audio::ISink* Server::make_audio_sink() {
    return &channel_muxer;
}

audio::IStreamReader* Server::make_audio_reader() {
    return &channel_muxer;
}

audio::ISampleBufferWriter* Server::make_audio_writer() {
    audio::ISampleBufferWriter* audio_writer = &output_writer;

    if (config().options & EnableTiming) {
        audio_writer = new (timed_writer)
            audio::TimedWriter(*audio_writer, config().channels, config().sample_rate);
    }

    return audio_writer;
}

} // namespace pipeline
} // namespace roc
