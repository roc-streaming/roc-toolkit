/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/player.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/default.h"

namespace roc {
namespace sndio {

Player::Player(core::BufferPool<audio::sample_t>& buffer_pool,
               core::IAllocator& allocator,
               bool oneshot,
               packet::channel_mask_t channels,
               size_t sample_rate)
    : output_(NULL)
    , buffer_pool_(buffer_pool)
    , allocator_(allocator)
    , n_bufs_(0)
    , oneshot_(oneshot)
    , is_file_(false) {
    size_t n_channels = packet::num_channels(channels);
    if (n_channels == 0) {
        roc_panic("player: # of channels is zero");
    }

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = sample_rate;
    out_signal_.channels = (unsigned)n_channels;
    out_signal_.precision = SOX_SAMPLE_PRECISION;

    buffer_size_ = sox_get_globals()->bufsiz;
}

Player::~Player() {
    if (joinable()) {
        roc_panic("player: destructor is called while thread is still running");
    }
    close_();
}

bool Player::open(const char* name, const char* type) {
    roc_log(LogDebug, "player: opening: name=%s type=%s", name, type);

    if (frame_buffer_ || sox_buffer_ || output_) {
        roc_panic("player: can't call open() more than once");
    }

    if (!prepare_()) {
        return false;
    }

    if (!open_(name, type)) {
        return false;
    }

    return true;
}

size_t Player::sample_rate() const {
    if (!output_) {
        roc_panic("player: sample_rate: non-open output file or device");
    }
    return size_t(output_->signal.rate);
}

bool Player::is_file() const {
    if (!output_) {
        roc_panic("player: is_file: non-open output file or device");
    }
    return is_file_;
}

bool Player::start(pipeline::IReceiver& input) {
    input_ = &input;
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

    if (!input_) {
        roc_panic("player: thread is started not from the start() call");
    }

    if (!output_) {
        roc_panic("player: thread is started before open() returned success");
    }

    loop_();
    close_();

    roc_log(LogDebug, "player: finishing thread, wrote %lu buffers",
            (unsigned long)n_bufs_);
}

bool Player::prepare_() {
    if (buffer_pool_.buffer_size() < buffer_size_) {
        roc_log(LogError, "player: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)buffer_size_, (unsigned long)buffer_pool_.buffer_size());
        return false;
    }

    frame_buffer_ = new (buffer_pool_) core::Buffer<audio::sample_t>(buffer_pool_);

    if (!frame_buffer_) {
        roc_log(LogError, "player: can't allocate frame buffer");
        return false;
    }

    frame_buffer_.resize(buffer_size_);

    sox_buffer_.reset(new (allocator_) sox_sample_t[buffer_size_], allocator_);

    if (!sox_buffer_) {
        roc_log(LogError, "player: can't allocate sox buffer");
        return false;
    }

    return true;
}

bool Player::open_(const char* name, const char* type) {
    if (!detect_defaults(&name, &type)) {
        roc_log(LogError, "player: can't detect defaults: name=%s type=%s", name, type);
        return false;
    }

    roc_log(LogInfo, "player: name=%s type=%s", name, type);

    output_ = sox_open_write(name, &out_signal_, NULL, type, NULL, NULL);
    if (!output_) {
        roc_log(LogError, "player: can't open writer: name=%s type=%s", name, type);
        return false;
    }

    is_file_ = !(output_->handler.flags & SOX_FILE_DEVICE);

    unsigned long in_rate = (unsigned long)out_signal_.rate;
    unsigned long out_rate = (unsigned long)output_->signal.rate;

    if (in_rate != 0 && in_rate != out_rate) {
        roc_log(LogError,
                "can't open output file or device with the required sample rate: "
                "required=%lu suggested=%lu",
                out_rate, in_rate);
        return false;
    }

    roc_log(LogInfo, "player:"
                     " bits=%lu out_rate=%lu in_rate=%lu ch=%lu is_file=%d",
            (unsigned long)output_->encoding.bits_per_sample, out_rate, in_rate,
            (unsigned long)output_->signal.channels, (int)is_file_);

    return true;
}

void Player::loop_() {
    sox_sample_t* soxbuf = sox_buffer_.get();
    size_t soxbuf_pos = 0;

    SOX_SAMPLE_LOCALS;

    size_t clips = 0;

    while (!stop_) {
        if (input_->status() == pipeline::IReceiver::Inactive) {
            if (oneshot_ && n_bufs_ != 0) {
                roc_log(LogInfo, "player: got inactive status, exiting");
                return;
            }
        } else {
            n_bufs_++;
        }

        audio::Frame frame(frame_buffer_.data(), frame_buffer_.size());
        input_->read(frame);

        const audio::sample_t* samples = frame.data();
        size_t n_samples = frame.size();

        while (n_samples > 0) {
            for (; soxbuf_pos < buffer_size_ && n_samples > 0; soxbuf_pos++) {
                soxbuf[soxbuf_pos] = SOX_FLOAT_32BIT_TO_SAMPLE(*samples, clips);
                samples++;
                n_samples--;
            }

            if (soxbuf_pos == buffer_size_) {
                if (!write_(soxbuf, soxbuf_pos)) {
                    return;
                }
                soxbuf_pos = 0;
            }
        }
    }

    if (!write_(soxbuf, soxbuf_pos)) {
        return;
    }
}

bool Player::write_(const sox_sample_t* samples, size_t n_samples) {
    if (n_samples > 0) {
        if (sox_write(output_, samples, n_samples) != n_samples) {
            roc_log(LogError, "player: can't write output buffer, exiting");
            return false;
        }
    }
    return true;
}

void Player::close_() {
    if (!output_) {
        return;
    }

    roc_log(LogDebug, "player: closing output");

    int err = sox_close(output_);
    if (err != SOX_SUCCESS) {
        roc_panic("player: can't close output: %s", sox_strerror(err));
    }

    output_ = NULL;
}

} // namespace sndio
} // namespace roc
