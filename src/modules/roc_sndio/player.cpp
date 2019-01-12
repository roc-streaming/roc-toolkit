/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/player.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

Player::Player(core::BufferPool<audio::sample_t>& buffer_pool,
               pipeline::IReceiver& input,
               audio::IWriter& output,
               size_t frame_size,
               bool oneshot)
    : input_(input)
    , output_(output)
    , n_bufs_(0)
    , oneshot_(oneshot)
    , stop_(0) {
    if (buffer_pool.buffer_size() < frame_size) {
        roc_log(LogError, "player: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)frame_size, (unsigned long)buffer_pool.buffer_size());
        return;
    }

    frame_buffer_ = new (buffer_pool) core::Buffer<audio::sample_t>(buffer_pool);

    if (!frame_buffer_) {
        roc_log(LogError, "player: can't allocate frame buffer");
        return;
    }

    frame_buffer_.resize(frame_size);
}

Player::~Player() {
    if (joinable()) {
        roc_panic("player: destructor is called while thread is still running");
    }
}

bool Player::valid() const {
    return frame_buffer_;
}

bool Player::start() {
    return Thread::start();
}

void Player::stop() {
    stop_ = 1;
}

void Player::join() {
    Thread::join();
}

void Player::run() {
    roc_log(LogDebug, "player: starting thread");

    while (!stop_) {
        if (input_.status() == pipeline::IReceiver::Inactive) {
            if (oneshot_ && n_bufs_ != 0) {
                roc_log(LogInfo, "player: got inactive status, exiting");
                return;
            }
        } else {
            n_bufs_++;
        }

        audio::Frame frame(frame_buffer_.data(), frame_buffer_.size());
        input_.read(frame);
        output_.write(frame);
    }

    roc_log(LogDebug, "player: finishing thread, wrote %lu buffers",
            (unsigned long)n_bufs_);
}

} // namespace sndio
} // namespace roc
