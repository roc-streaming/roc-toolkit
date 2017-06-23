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

void print_packet(const IPacket& p, bool print_payload) {
    enum { MaxPerLine = 10 };

    fprintf(stderr, "packet: raw_sz=%lu payload_sz=%lu",
            (unsigned long)p.raw_data().size(), //
            (unsigned long)p.payload().size());

    if (p.rtp()) {
        fprintf(stderr, " rtp: src=%lu m=%d, sn=%u, ts=%lu\n",
                (unsigned long)p.rtp()->source(), //
                (int)p.rtp()->marker(),           //
                (unsigned)p.rtp()->seqnum(),      //
                (unsigned long)p.rtp()->timestamp());
    }

    if (p.fec()) {
        fprintf(stderr, " fec: data_blk=%u, fec_blk=%u\n",
                (unsigned)p.fec()->source_blknum(), //
                (unsigned)p.fec()->repair_blknum());

        if (print_payload) {
            for (size_t n = 0; n < p.payload().size(); n++) {
                if (n % MaxPerLine == 0) {
                    fprintf(stderr, "\n  ");
                }
                fprintf(stderr, " %02x", (unsigned)p.payload().data()[n]);
            }

            fprintf(stderr, "\n");
        }
    }

    if (p.audio()) {
        channel_mask_t channels = p.audio()->channels();
        size_t ns = p.audio()->num_samples();

        fprintf(stderr, " audio: ch=0x%x, ns=%u\n",
                (unsigned)channels, //
                (unsigned)ns);

        if (print_payload) {
            for (channel_t ch = 0; ch < sizeof(channels) * 8; ch++) {
                if (channels & (1 << ch)) {
                    fprintf(stderr, "  ch %u:", (unsigned)ch);

                    for (size_t n = 0; n < ns; n++) {
                        sample_t s = 0;
                        p.audio()->read_samples((1 << ch), n, &s, 1);

                        if (n % MaxPerLine == 0) {
                            fprintf(stderr, "\n  ");
                        }
                        fprintf(stderr, " %.3f", (double)s);
                    }

                    fprintf(stderr, "\n");
                }
            }
        }
    }
}

} // namespace packet
} // namespace roc
