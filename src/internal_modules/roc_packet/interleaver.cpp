/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/interleaver.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"

namespace roc {
namespace packet {

Interleaver::Interleaver(IWriter& writer, core::IAllocator& allocator, size_t block_sz)
    : writer_(writer)
    , block_size_(block_sz)
    , send_seq_(allocator)
    , packets_(allocator)
    , next_2_put_(0)
    , next_2_send_(0)
    , valid_(false) {
    roc_panic_if(block_sz == 0);

    if (!send_seq_.resize(block_size_)) {
        return;
    }
    if (!packets_.resize(block_size_)) {
        return;
    }

    reinit_seq_();

    roc_log(LogDebug, "initializing interleaver: block_size=%u", (unsigned)block_size_);

    for (size_t i = 0; i < block_size_; ++i) {
        roc_log(LogTrace, "  interleaver_seq[%u]: %u", (unsigned)i,
                (unsigned)send_seq_[i]);
    }

    valid_ = true;
}

bool Interleaver::valid() const {
    return valid_;
}

void Interleaver::write(const PacketPtr& p) {
    roc_panic_if_not(valid());

    packets_[next_2_put_] = p;
    next_2_put_ = (next_2_put_ + 1) % block_size_;

    while (packets_[send_seq_[next_2_send_]]) {
        writer_.write(packets_[send_seq_[next_2_send_]]);
        packets_[send_seq_[next_2_send_]] = NULL;
        next_2_send_ = (next_2_send_ + 1) % block_size_;
    }
}

void Interleaver::flush() {
    roc_panic_if_not(valid());

    for (size_t i = 0; i < block_size_; ++i) {
        if (packets_[i]) {
            writer_.write(packets_[i]);
            packets_[i] = NULL;
        }
    }

    next_2_put_ = next_2_send_ = 0;
}

size_t Interleaver::block_size() const {
    return block_size_;
}

void Interleaver::reinit_seq_() {
    for (size_t i = 0; i < block_size_; ++i) {
        send_seq_[i] = i;
    }
    for (size_t i = block_size_; i > 0; --i) {
        const size_t j = core::fast_random(0, (unsigned int)i - 1);
        const size_t buff = send_seq_[i - 1];
        send_seq_[i - 1] = send_seq_[j];
        send_seq_[j] = buff;
    }
}

} // namespace packet
} // namespace roc
