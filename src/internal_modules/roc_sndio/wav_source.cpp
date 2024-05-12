/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/wav_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

WavSource::WavSource(core::IArena& arena, const Config& config)
    : file_opened_(false)
    , eof_(false)
    , valid_(false) {
    if (config.latency != 0) {
        roc_log(LogError, "wav source: setting io latency not supported");
        return;
    }

    if (!config.sample_spec.is_empty()) {
        roc_log(LogError, "wav source: setting io encoding not supported");
        return;
    }

    valid_ = true;
}

WavSource::~WavSource() {
    close_();
}

bool WavSource::is_valid() const {
    return valid_;
}

bool WavSource::open(const char* path) {
    roc_panic_if(!valid_);

    if (!open_(path)) {
        return false;
    }

    return true;
}

ISink* WavSource::to_sink() {
    return NULL;
}

ISource* WavSource::to_source() {
    return this;
}

DeviceType WavSource::type() const {
    return DeviceType_Source;
}

DeviceState WavSource::state() const {
    return DeviceState_Active;
}

void WavSource::pause() {
    // no-op
}

bool WavSource::resume() {
    return true;
}

bool WavSource::restart() {
    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    roc_log(LogDebug, "wav source: restarting");

    if (!drwav_seek_to_pcm_frame(&wav_, 0)) {
        roc_log(LogError, "wav source: seek failed when restarting");
        return false;
    }

    eof_ = false;

    return true;
}

audio::SampleSpec WavSource::sample_spec() const {
    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    audio::ChannelSet channel_set;
    channel_set.set_layout(audio::ChanLayout_Surround);
    channel_set.set_order(audio::ChanOrder_Smpte);
    channel_set.set_count(wav_.channels);

    return audio::SampleSpec(size_t(wav_.sampleRate), audio::Sample_RawFormat,
                             channel_set);
}

core::nanoseconds_t WavSource::latency() const {
    return 0;
}

bool WavSource::has_latency() const {
    return false;
}

bool WavSource::has_clock() const {
    return false;
}

void WavSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode WavSource::read(audio::Frame& frame) {
    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    if (eof_) {
        return status::StatusEnd;
    }

    audio::sample_t* frame_data = frame.raw_samples();
    size_t frame_left = frame.num_raw_samples();

    while (frame_left != 0) {
        size_t n_samples = frame_left;

        n_samples =
            drwav_read_pcm_frames_f32(&wav_, n_samples / wav_.channels, frame_data)
            * wav_.channels;

        if (n_samples == 0) {
            roc_log(LogDebug, "wav source: got eof from input file");
            eof_ = true;
            break;
        }

        frame_data += n_samples;
        frame_left -= n_samples;
    }

    if (frame_left == frame.num_raw_samples()) {
        return status::StatusEnd;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return status::StatusOK;
}

bool WavSource::open_(const char* path) {
    if (file_opened_) {
        roc_panic("wav source: already opened");
    }

    if (!drwav_init_file(&wav_, path, NULL)) {
        roc_log(LogDebug, "wav sink: can't open input file: %s",
                core::errno_to_str(errno).c_str());
        return false;
    }

    roc_log(LogInfo,
            "wav source: opened input file:"
            " path=%s in_bits=%lu in_rate=%lu in_ch=%lu",
            path, (unsigned long)wav_.bitsPerSample, (unsigned long)wav_.sampleRate,
            (unsigned long)wav_.channels);

    file_opened_ = true;
    return true;
}

void WavSource::close_() {
    if (!file_opened_) {
        return;
    }

    file_opened_ = false;
    drwav_uninit(&wav_);
}

} // namespace sndio
} // namespace roc
