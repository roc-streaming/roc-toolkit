/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/composer.h"
#include "roc_core/align_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtcp {

Composer::Composer(core::IArena& arena)
    : IComposer(arena) {
}

ROC_ATTR_NODISCARD status::StatusCode Composer::init_status() const {
    return status::StatusOK;
}

ROC_ATTR_NODISCARD status::StatusCode Composer::align(core::Slice<uint8_t>& buffer,
                                                      size_t header_size,
                                                      size_t payload_alignment) {
    if ((unsigned long)buffer.data() % payload_alignment != 0) {
        roc_panic("rtcp composer: unexpected non-aligned buffer");
    }

    const size_t padding = core::AlignOps::pad_as(header_size, payload_alignment);

    if (buffer.capacity() < padding) {
        roc_log(LogDebug,
                "rtcp composer: not enough space for alignment: padding=%lu cap=%lu",
                (unsigned long)padding, (unsigned long)buffer.capacity());
        return status::StatusBadBuffer;
    }

    buffer.reslice(padding, padding);
    return status::StatusOK;
}

ROC_ATTR_NODISCARD status::StatusCode Composer::prepare(packet::Packet& packet,
                                                        core::Slice<uint8_t>& buffer,
                                                        size_t payload_size) {
    buffer.reslice(0, payload_size);

    packet.add_flags(packet::Packet::FlagControl);
    packet.add_flags(packet::Packet::FlagRTCP);

    packet.rtcp()->payload = buffer;

    return status::StatusOK;
}

ROC_ATTR_NODISCARD status::StatusCode Composer::pad(packet::Packet& packet,
                                                    size_t padding_size) {
    // not supported

    (void)packet;
    (void)padding_size;

    return status::StatusBadOperation;
}

ROC_ATTR_NODISCARD status::StatusCode Composer::compose(packet::Packet& packet) {
    if (!packet.rtcp()) {
        roc_panic("rtcp composer: unexpected non-rctp packet");
    }

    if (!packet.rtcp()->payload) {
        roc_panic("rtcp composer: unexpected null data");
    }

    if (packet.rtcp()->payload.size() == 0) {
        roc_panic("rtcp composer: unexpected zero data");
    }

    return status::StatusOK;
}

} // namespace rtcp
} // namespace roc
