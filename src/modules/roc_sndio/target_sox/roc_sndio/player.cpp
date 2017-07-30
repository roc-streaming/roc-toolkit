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
#include "roc_core/unique_ptr.h"
#include "roc_sndio/default.h"

namespace roc {
namespace sndio {

Player::Player(pipeline::IReceiver& input,
               core::BufferPool<audio::sample_t>& buffer_pool,
               core::IAllocator& allocator,
               bool oneshot,
               packet::channel_mask_t channels,
               size_t sample_rate)
    : output_(NULL)
    , input_(input)
    , buffer_pool_(buffer_pool)
    , allocator_(allocator)
    , clips_(0)
    , n_bufs_(0)
    , oneshot_(oneshot) {
    size_t n_channels = packet::num_channels(channels);
    if (n_channels == 0) {
        roc_panic("player: # of channels is zero");
    }

    if (sample_rate == 0) {
        roc_panic("player: sample rate is zero");
    }

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = sample_rate;
    out_signal_.channels = (unsigned)n_channels;
    out_signal_.precision = SOX_SAMPLE_PRECISION;
}

Player::~Player() {
    if (joinable()) {
        roc_panic("player: destructor is called while thread is still running");
    }
    close_();
}

bool Player::open(const char* name, const char* type) {
    roc_log(LogDebug, "player: opening: name=%s type=%s", name, type);

    if (output_) {
        roc_panic("player: can't call open() more than once");
    }

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

    return true;
}

void Player::stop() {
    stop_ = 1;
}

void Player::run() {
    roc_log(LogDebug, "player: starting thread");

    if (!output_) {
        roc_panic("player: thread is started before open() returnes success");
    }

    loop_();
    close_();

    roc_log(LogDebug, "player: finishing thread, wrote %lu buffers",
            (unsigned long)n_bufs_);
}

void Player::loop_() {
    const size_t outbuf_sz = sox_get_globals()->bufsiz;

    core::UniquePtr<sox_sample_t> outptr(new (allocator_) sox_sample_t[outbuf_sz],
                                         allocator_);

    if (!outptr) {
        roc_panic("player: can't allocate output buffer");
    }

    sox_sample_t* outbuf = outptr.get();
    size_t outbuf_pos = 0;

    audio::Frame frame;
    frame.samples = new (buffer_pool_) core::Buffer<audio::sample_t>(buffer_pool_);
    if (!frame.samples) {
        roc_panic("player: can't allocate input buffer");
    }
    frame.samples.resize(outbuf_sz);

    SOX_SAMPLE_LOCALS;

    while (!stop_) {
        pipeline::IReceiver::Status status = input_.read(frame);

        if (status == pipeline::IReceiver::Inactive) {
            if (oneshot_ && n_bufs_ != 0) {
                roc_log(LogInfo, "player: got inactive status, exiting");
                return;
            }
        } else {
            n_bufs_++;
        }

        const audio::sample_t* samples = frame.samples.data();
        size_t n_samples = frame.samples.size();
        roc_panic_if(n_samples != outbuf_sz);

        while (n_samples > 0) {
            for (; outbuf_pos < outbuf_sz && n_samples > 0; outbuf_pos++) {
                outbuf[outbuf_pos] = SOX_FLOAT_32BIT_TO_SAMPLE(*samples, clips_);
                samples++;
                n_samples--;
            }

            if (outbuf_pos == outbuf_sz) {
                if (!write_(outbuf, outbuf_sz)) {
                    return;
                }
                outbuf_pos = 0;
            }
        }
    }

    if (!write_(outbuf, outbuf_pos)) {
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
