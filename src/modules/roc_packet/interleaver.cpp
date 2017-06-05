/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/interleaver.h"
#include "roc_core/log.h"
#include "roc_core/random.h"

namespace roc {
namespace packet {

Interleaver::Interleaver(packet::IPacketWriter& output, const size_t delay_len)
    : output_(output)
    , delay_len_(delay_len)
    , pack_store_(delay_len) {
    roc_panic_if(delay_len == 0);

    roc_log(LogDebug, "initializing interleaver");

    reinit_seq();

    roc_log(LogDebug, "interleaver block delay_len_: %u", (unsigned)delay_len_);
    for (size_t i = 0; i < delay_len_; ++i) {
        roc_log(LogDebug, "\tinterleaver_seq[%u]: %u", (unsigned)i, (unsigned)tx_seq_[i]);
    }

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

void Interleaver::reinit_seq() {
    for (size_t i = 0; i < delay_len_; ++i) {
        tx_seq_[i] = i;
    }
    for (size_t i = delay_len_; i > 0; --i) {
        const size_t j = core::random(0, (unsigned int)i - 1);
        const size_t buff = tx_seq_[i - 1];
        tx_seq_[i - 1] = tx_seq_[j];
        tx_seq_[j] = buff;
    }
}

} // namespace packet
} // namespace roc
