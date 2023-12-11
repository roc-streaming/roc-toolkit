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
    : eof_(false)
    , paused_(false)
    , valid_(false) {
    (void)arena;

    if (config.sample_spec.num_channels() == 0) {
        roc_log(LogError, "wav source: # of channels is zero");
        return;
    }

    if (config.latency != 0) {
        roc_log(LogError, "wav source: setting io latency not supported by wav backend");
        return;
    }

    frame_length_ = config.frame_length;

    if (frame_length_ == 0) {
        roc_log(LogError, "wav source: frame length is zero");
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

    roc_log(LogInfo, "wav source: opening: path=%s", path);

    if (file_opened_) {
        roc_panic("wav source: can't call open() more than once");
    }

    if (!open_(path)) {
        return false;
    }

    file_opened_ = true;
    return true;
}

DeviceType WavSource::type() const {
    return DeviceType_Source;
}

DeviceState WavSource::state() const {
    roc_panic_if(!valid_);

    if (paused_) {
        return DeviceState_Paused;
    } else {
        return DeviceState_Active;
    }
}

void WavSource::pause() {
    // no-op - but the state is updated
    paused_ = true;
}

bool WavSource::resume() {
    // no-op - but the state is updated
    paused_ = false;
    return true;
}

bool WavSource::restart() {
    roc_panic_if(!valid_);
    roc_panic_if(!file_opened_);

    roc_log(LogDebug, "wav source: restarting");

    if (!seek_(0)) {
        roc_log(LogError, "wav source: seek failed when restarting");
        return false;
    }

    paused_ = false;
    eof_ = false;

    return true;
}

audio::SampleSpec WavSource::sample_spec() const {
    roc_panic_if(!valid_);

    if (!file_opened_) {
        roc_panic("wav source: sample_spec(): non-open output file or device");
    }

    audio::ChannelSet channel_set;
    channel_set.set_layout(audio::ChanLayout_Surround);
    channel_set.set_order(audio::ChanOrder_Smpte);
    channel_set.set_channel_range(0, wav_.channels - 1, true);

    return audio::SampleSpec(size_t(wav_.sampleRate), audio::PcmFormat_Float32_Le,
                             channel_set);
}

core::nanoseconds_t WavSource::latency() const {
    return 0;
}

bool WavSource::has_latency() const {
    roc_panic_if(!valid_);

    if (!file_opened_) {
        roc_panic("wav source: has_latency(): non-open input file or device");
    }

    return false;
}

bool WavSource::has_clock() const {
    roc_panic_if(!valid_);

    if (!file_opened_) {
        roc_panic("wav source: has_clock(): non-open input file or device");
    }

    return false;
}

void WavSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
    (void)timestamp;
}

bool WavSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (paused_ || eof_) {
        return false;
    }

    if (!file_opened_) {
        roc_panic("wav source: read: non-open input file");
    }

    audio::sample_t* frame_data = frame.samples();
    size_t frame_left = frame.num_samples();

    while (frame_left != 0) {
        size_t n_samples = frame_left;

        n_samples =
            drwav_read_pcm_frames_f32(&wav_, n_samples / wav_.channels, frame_data)
            * wav_.channels;

        if (n_samples == 0) {
            roc_log(LogDebug, "wav source: got eof from wav");
            eof_ = true;
            break;
        }

        frame_data += n_samples;
        frame_left -= n_samples;
    }

    if (frame_left == frame.num_samples()) {
        return false;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return true;
}

bool WavSource::open_(const char* filename) {
    if (file_opened_) {
        roc_panic("wav source: already opened");
    }

    if (!drwav_init_file(&wav_, filename, NULL)) {
        roc_log(LogInfo, "wav source: can't open: input=%s", filename);
        return false;
    }

    roc_log(LogInfo,
            "wav source:"
            " in_bits=%lu in_rate=%lu in_ch=%lu",
            (unsigned long)wav_.bitsPerSample, (unsigned long)wav_.sampleRate,
            (unsigned long)wav_.channels);

    return true;
}

void WavSource::close_() {
    if (!file_opened_) {
        return;
    }

    file_opened_ = false;
    drwav_uninit(&wav_);
}

bool WavSource::seek_(drwav_uint64 target_frame_index) {
    return drwav_seek_to_pcm_frame(&wav_, target_frame_index);
}

} // namespace sndio
} // namespace roc
