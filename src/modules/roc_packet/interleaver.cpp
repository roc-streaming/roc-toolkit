/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_packet/interleaver.h"

namespace roc {
namespace packet {

const size_t Interleaver::tx_seq_[delay_len_] = { 4, 2, 6, 0, 5, 1, 3, 8, 7 };

Interleaver::Interleaver(packet::IPacketWriter& output)
    : output_(output)
    , pack_store_(delay_len_) {
    roc_log(LogDebug, "initializing interleaver");

    next_2_send_ = 0;
    next_2_put_ = 0;
}

void Interleaver::write(const packet::IPacketPtr& p) {
    pack_store_[next_2_put_] = p;
    next_2_put_ = (next_2_put_ + 1) % delay_len_;
    while (pack_store_[tx_seq_[next_2_send_]]) {
        output_.write(pack_store_[tx_seq_[next_2_send_]]);
        pack_store_[tx_seq_[next_2_send_]] = NULL;

        next_2_send_ = (next_2_send_ + 1) % delay_len_;
    }
}

void Interleaver::flush() {
    for (size_t i = 0; i < delay_len_; ++i) {
        if (pack_store_[i]) {
            output_.write(pack_store_[i]);
            pack_store_[i] = NULL;
        }
    }
    next_2_put_ = next_2_send_ = 0;
}

size_t Interleaver::window_size() const {
    return delay_len_;
}

} // namespace packet
} // namespace roc
