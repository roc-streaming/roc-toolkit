/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/interleaver.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace packet {

Interleaver::Interleaver(IWriter& writer, core::IArena& arena, size_t block_sz)
    : writer_(writer)
    , block_size_(block_sz)
    , send_seq_(arena)
    , packets_(arena)
    , next_2_put_(0)
    , next_2_send_(0)
    , init_status_(status::NoStatus) {
    roc_panic_if(block_sz == 0);

    if (!send_seq_.resize(block_size_)) {
        init_status_ = status::StatusNoMem;
        return;
    }
    if (!packets_.resize(block_size_)) {
        init_status_ = status::StatusNoMem;
        return;
    }

    reinit_seq_();

    roc_log(LogDebug, "initializing interleaver: block_size=%u", (unsigned)block_size_);

    for (size_t i = 0; i < block_size_; ++i) {
        roc_log(LogTrace, "  interleaver_seq[%u]: %u", (unsigned)i,
                (unsigned)send_seq_[i]);
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Interleaver::init_status() const {
    return init_status_;
}

status::StatusCode Interleaver::write(const PacketPtr& p) {
    roc_panic_if(init_status_ != status::StatusOK);

    packets_[next_2_put_] = p;
    next_2_put_ = (next_2_put_ + 1) % block_size_;

    while (packets_[send_seq_[next_2_send_]]) {
        const status::StatusCode code = writer_.write(packets_[send_seq_[next_2_send_]]);
        if (code != status::StatusOK) {
            return code;
        }

        packets_[send_seq_[next_2_send_]] = NULL;
        next_2_send_ = (next_2_send_ + 1) % block_size_;
    }

    return status::StatusOK;
}

status::StatusCode Interleaver::flush() {
    roc_panic_if(init_status_ != status::StatusOK);

    for (size_t i = 0; i < block_size_; ++i) {
        if (!packets_[i]) {
            continue;
        }

        const status::StatusCode code = writer_.write(packets_[i]);
        if (code != status::StatusOK) {
            return code;
        }

        packets_[i] = NULL;
    }

    next_2_put_ = next_2_send_ = 0;

    return status::StatusOK;
}

size_t Interleaver::block_size() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return block_size_;
}

void Interleaver::reinit_seq_() {
    for (size_t i = 0; i < block_size_; ++i) {
        send_seq_[i] = i;
    }
    for (size_t i = block_size_; i > 0; --i) {
        const size_t j = core::fast_random_range(0, (unsigned int)i - 1);
        const size_t buff = send_seq_[i - 1];
        send_seq_[i - 1] = send_seq_[j];
        send_seq_[j] = buff;
    }
}

} // namespace packet
} // namespace roc
