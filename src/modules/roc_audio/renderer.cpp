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

#include "roc_audio/renderer.h"

namespace roc {
namespace audio {

Renderer::Renderer()
    : readers_(MaxChannels) {
}

void Renderer::set_reader(packet::channel_t ch, IStreamReader& reader) {
    roc_panic_if(&reader == NULL);

    if (readers_[ch]) {
        roc_panic("renderer: attempting to overwrite stream for channel %u",
                  (unsigned)ch);
    }

    readers_[ch] = &reader;
}

void Renderer::add_tuner(ITuner& tuner) {
    roc_panic_if(&tuner == NULL);

    tuners_.append(tuner);
}

bool Renderer::update() {
    ITuner* tuner = tuners_.front();

    for (; tuner != NULL; tuner = tuners_.next(*tuner)) {
        if (!tuner->update()) {
            roc_log(LOG_DEBUG, "renderer: tuner returned error");
            return false;
        }
    }

    return true;
}

void Renderer::attach(ISink& sink) {
    roc_log(LOG_TRACE, "renderer: attaching readers to sink");

    for (size_t ch = 0; ch < readers_.size(); ch++) {
        if (readers_[ch]) {
            sink.attach(packet::channel_t(ch), *readers_[ch]);
        }
    }
}

void Renderer::detach(ISink& sink) {
    roc_log(LOG_TRACE, "renderer: detaching readers from sink");

    for (size_t ch = 0; ch < readers_.size(); ch++) {
        if (readers_[ch]) {
            sink.detach(packet::channel_t(ch), *readers_[ch]);
        }
    }
}

} // namespace audio
} // namespace roc
