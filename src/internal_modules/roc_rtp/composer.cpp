/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/composer.h"
#include "roc_core/align_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

Composer::Composer(packet::IComposer* inner_composer, core::IArena& arena)
    : IComposer(arena)
    , inner_composer_(inner_composer) {
}

ROC_NODISCARD status::StatusCode Composer::init_status() const {
    return status::StatusOK;
}

ROC_NODISCARD status::StatusCode Composer::align(core::Slice<uint8_t>& buffer,
                                                 size_t header_size,
                                                 size_t payload_alignment) {
    if ((unsigned long)buffer.data() % payload_alignment != 0) {
        roc_panic("rtp composer: unexpected non-aligned buffer");
    }

    header_size += sizeof(Header);

    if (inner_composer_ == NULL) {
        const size_t padding = core::AlignOps::pad_as(header_size, payload_alignment);

        if (buffer.capacity() < padding) {
            roc_log(LogDebug,
                    "rtp composer: not enough space for alignment: padding=%lu cap=%lu",
                    (unsigned long)padding, (unsigned long)buffer.capacity());
            return status::StatusBadBuffer;
        }

        buffer.reslice(padding, padding);
        return status::StatusOK;
    } else {
        return inner_composer_->align(buffer, header_size, payload_alignment);
    }
}

ROC_NODISCARD status::StatusCode Composer::prepare(packet::Packet& packet,
                                                   core::Slice<uint8_t>& buffer,
                                                   size_t payload_size) {
    core::Slice<uint8_t> header = buffer.subslice(0, 0);

    if (header.capacity() < sizeof(Header)) {
        roc_log(LogDebug,
                "rtp composer: not enough space for rtp header: size=%lu cap=%lu",
                (unsigned long)sizeof(Header), (unsigned long)header.capacity());
        return status::StatusBadBuffer;
    }
    header.reslice(0, sizeof(Header));

    core::Slice<uint8_t> payload = header.subslice(header.size(), header.size());

    if (inner_composer_ == NULL) {
        if (payload.capacity() < payload_size) {
            roc_log(LogDebug,
                    "rtp composer: not enough space for rtp payload: size=%lu cap=%lu",
                    (unsigned long)payload_size, (unsigned long)payload.capacity());
            return status::StatusBadBuffer;
        }
        payload.reslice(0, payload_size);
    } else {
        status::StatusCode status =
            inner_composer_->prepare(packet, payload, payload_size);
        if (status != status::StatusOK) {
            return status;
        }
    }

    packet.add_flags(packet::Packet::FlagRTP);

    packet::RTP& rtp = *packet.rtp();

    rtp.header = header;
    rtp.payload = payload;

    buffer.reslice(0, header.size() + payload.size());

    return status::StatusOK;
}

ROC_NODISCARD status::StatusCode Composer::pad(packet::Packet& packet,
                                               size_t padding_size) {
    if (inner_composer_) {
        return inner_composer_->pad(packet, padding_size);
    }

    packet::RTP* rtp = packet.rtp();
    if (!rtp) {
        roc_panic("rtp composer: unexpected non-rtp packet");
    }

    if (rtp->padding) {
        roc_panic("rtp composer: can't pad packet twice");
    }

    const size_t payload_size = rtp->payload.size();

    if (payload_size < padding_size) {
        roc_log(LogDebug,
                "rtp composer: padding is larger than payload size:"
                " payload_size=%lu padding_size=%lu",
                (unsigned long)rtp->payload.size(), (unsigned long)padding_size);
        return status::StatusBadBuffer;
    }

    rtp->padding = rtp->payload.subslice(payload_size - padding_size, payload_size);
    rtp->payload = rtp->payload.subslice(0, payload_size - padding_size);

    return status::StatusOK;
}

ROC_NODISCARD status::StatusCode Composer::compose(packet::Packet& packet) {
    packet::RTP* rtp = packet.rtp();
    if (!rtp) {
        roc_panic("rtp composer: unexpected non-rtp packet");
    }

    if (rtp->header.size() != sizeof(Header)) {
        roc_panic("rtp composer: unexpected rtp header size");
    }

    Header& header = *(Header*)rtp->header.data();

    header.clear();
    header.set_version(V2);
    header.set_ssrc(rtp->source_id);
    header.set_seqnum(rtp->seqnum);
    header.set_timestamp(rtp->stream_timestamp);
    header.set_marker(rtp->marker);
    header.set_payload_type(PayloadType(rtp->payload_type));

    if (rtp->padding.size() > 0) {
        header.set_padding(true);

        uint8_t* padding_data = rtp->padding.data();
        size_t padding_size = rtp->padding.size();

        if (padding_size > (uint8_t)-1) {
            roc_log(LogDebug,
                    "rtp composer: padding is larger than supported by rtp:"
                    " pad_size=%lu max_size=%lu",
                    (unsigned long)padding_size, (unsigned long)(uint8_t)-1);
            return status::StatusBadBuffer;
        }

        if (padding_size > 1) {
            memset(padding_data, 0, padding_size - 1);
        }
        padding_data[padding_size - 1] = (uint8_t)padding_size;
    }

    if (inner_composer_) {
        return inner_composer_->compose(packet);
    }

    return status::StatusOK;
}

} // namespace rtp
} // namespace roc
