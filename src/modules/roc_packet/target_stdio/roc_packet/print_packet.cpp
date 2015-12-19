/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdio>

#include "roc_packet/print_packet.h"

namespace roc {
namespace packet {

void print_packet(const IAudioPacket& p, bool body) {
    enum { MaxPerLine = 10 };

    channel_mask_t channels = p.channels();
    size_t ns = p.num_samples();

    fprintf(stderr, "packet(audio): m=%d, sn=%u, ts=%lu, ch=0x%x, ns=%u\n",
            (int)p.marker(), (unsigned)p.seqnum(), (unsigned long)p.timestamp(),
            (unsigned)channels, (unsigned)ns);

    if (!body) {
        return;
    }

    for (channel_t ch = 0; ch < sizeof(channels) * 8; ch++) {
        if (channels & (1 << ch)) {
            fprintf(stderr, " ch %u:", (unsigned)ch);

            for (size_t n = 0; n < ns; n++) {
                sample_t s = 0;
                p.read_samples((1 << ch), n, &s, 1);

                if (n % MaxPerLine == 0) {
                    fprintf(stderr, "\n ");
                }
                fprintf(stderr, " %.3f", (double)s);
            }

            fprintf(stderr, "\n");
        }
    }
}

void print_packet(const IFECPacket& p, bool body) {
    enum { MaxPerLine = 16 };

    core::IByteBufferConstSlice payload = p.payload();

    fprintf(stderr, "packet(fec): m=%d, sn=%u, data_blk=%u, fec_blk=%u, payload=%u\n",
            (int)p.marker(), (unsigned)p.seqnum(), (unsigned)p.data_blknum(),
            (unsigned)p.fec_blknum(), (unsigned)payload.size());

    if (!body) {
        return;
    }

    for (size_t n = 0; n < payload.size(); n++) {
        if (n % MaxPerLine == 0) {
            fprintf(stderr, "\n ");
        }
        fprintf(stderr, " %02x", (unsigned)payload.data()[n]);
    }

    fprintf(stderr, "\n");
}

} // namespace packet
} // namespace roc
