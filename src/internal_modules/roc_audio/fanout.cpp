/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/fanout.h"

namespace roc {
namespace audio {

bool Fanout::has_output(IFrameWriter& writer) {
    return writers_.contains(writer);
}

void Fanout::add_output(IFrameWriter& writer) {
    writers_.push_back(writer);
}

void Fanout::remove_output(IFrameWriter& writer) {
    writers_.remove(writer);
}

void Fanout::write(Frame& frame) {
    for (IFrameWriter* wp = writers_.front(); wp; wp = writers_.nextof(*wp)) {
        wp->write(frame);
    }
}

} // namespace audio
} // namespace roc
