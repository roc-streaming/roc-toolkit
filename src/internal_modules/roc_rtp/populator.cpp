/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/populator.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

Populator::Populator(packet::IReader& reader,
                     audio::IFrameDecoder& decoder,
                     const audio::SampleSpec& sample_spec)
    : reader_(reader)
    , decoder_(decoder)
    , sample_spec_(sample_spec) {
}

packet::PacketPtr Populator::read() {
    packet::PacketPtr packet = reader_.read();
    if (!packet) {
        return NULL;
    }

    if (!packet->rtp()) {
        roc_panic("rtp populator: unexpected non-rtp packet");
    }

    if (packet->rtp()->duration == 0) {
        packet->rtp()->duration =
            (packet::stream_timestamp_t)decoder_.decoded_sample_count(
                packet->rtp()->payload.data(), packet->rtp()->payload.size());
    }

    return packet;
}

} // namespace rtp
} // namespace roc
