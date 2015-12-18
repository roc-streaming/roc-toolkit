/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_pipeline/basic_client.h"

namespace roc {
namespace pipeline {

BasicClient::BasicClient(const ClientConfig& cfg,
                         datagram::IDatagramWriter& datagram_writer)
    : config_(cfg)
    , audio_reader_(NULL)
    , audio_writer_(NULL)
    , datagram_writer_(datagram_writer) {
}

const ClientConfig& BasicClient::config() const {
    return config_;
}

void BasicClient::run() {
    roc_log(LOG_DEBUG, "client: starting thread");

    for (;;) {
        if (!tick()) {
            break;
        }
    }

    roc_log(LOG_DEBUG, "client: finishing thread");

    datagram_writer_.write(NULL);
}

bool BasicClient::tick() {
    if (!audio_reader_) {
        if (!(audio_reader_ = make_audio_reader())) {
            roc_panic("client: make_audio_reader() returned null");
        }
    }

    if (!audio_writer_) {
        if (!(audio_writer_ = make_audio_writer())) {
            roc_panic("client: make_audio_writer() returned null");
        }
    }

    audio::ISampleBufferConstSlice buffer = audio_reader_->read();
    if (!buffer) {
        roc_log(LOG_DEBUG, "client: audio reader returned null");
        return false;
    }

    audio_writer_->write(buffer);

    return true;
}

} // namespace pipeline
} // namespace roc
