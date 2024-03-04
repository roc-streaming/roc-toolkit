/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"

namespace roc {
namespace sndio {

SndfileSource::SndfileSource(core::IArena& arena, const Config& config)
    : file_(NULL)
    , path_(NULL)
    , eof_(false)
    , valid_(false) {
    if (config.latency != 0) {
        roc_log(LogError,
                "sndfile source: setting io latency not supported by sndfile backend");
        return;
    }

    if (!config.sample_spec.is_empty()) {
        roc_log(LogError, "sndfile source: setting io encoding not supported");
        return;
    }

    frame_length_ = config.frame_length;
    sample_spec_ = config.sample_spec;

    if (frame_length_ == 0) {
        roc_log(LogError, "sndfile source: frame length is zero");
        return;
    }

    memset(&file_info_, 0, sizeof(file_info_));
    sample_rate_ = config.sample_spec.sample_rate();
    valid_ = true;
}

SndfileSource::~SndfileSource() {
    close_();
}

bool SndfileSource::is_valid() const {
    return valid_;
}

bool SndfileSource::open(const char* driver, const char* path) {
    roc_panic_if(!valid_);

    if (path) {
        path_ = path;
    }

    roc_log(LogInfo, "sndfile source: opening: driver=%s path=%s", driver, path);

    if (file_) {
        roc_panic("sndfile source: can't call open() more than once");
    }

    if (!open_(path)) {
        return false;
    }

    return true;
}

ISink* SndfileSource::to_sink() {
    return NULL;
}

ISource* SndfileSource::to_source() {
    return this;
}

DeviceType SndfileSource::type() const {
    return DeviceType_Source;
}

DeviceState SndfileSource::state() const {
    roc_panic_if(!valid_);
    return DeviceState_Active;
}

void SndfileSource::pause() {
    // no-op
}

bool SndfileSource::resume() {
    // no-op
    return true;
}

bool SndfileSource::restart() {
    roc_panic_if(!valid_);

    roc_log(LogDebug, "sndfile source: restarting");

    if (!eof_) {
        if (!seek_(0)) {
            roc_log(LogError, "sndfile source: seek failed when restarting");
            return false;
        }
    } else {
        if (file_) {
            close_();
        }

        if (!open_(path_)) {
            roc_log(LogError, "sndfile source: open failed when restarting");
            return false;
        }
    }

    eof_ = false;

    return true;
}

audio::SampleSpec SndfileSource::sample_spec() const {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: sample_rate(): non-open output file");
    }

    audio::ChannelSet channel_set;
    channel_set.set_layout(audio::ChanLayout_Surround);
    channel_set.set_order(audio::ChanOrder_Smpte);
    channel_set.set_channel_range(0, (size_t)file_info_.channels - 1, true);

    return audio::SampleSpec(size_t(file_info_.samplerate), audio::Sample_RawFormat,
                             channel_set);
}

core::nanoseconds_t SndfileSource::latency() const {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: latency(): non-open output file");
    }

    return 0;
}

bool SndfileSource::has_latency() const {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: has_latency(): non-open input file");
    }

    return false;
}

bool SndfileSource::has_clock() const {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: has_clock(): non-open input file");
    }

    return false;
}

void SndfileSource::reclock(core::nanoseconds_t) {
    // no-op
}

bool SndfileSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: read: non-open input file");
    }

    audio::sample_t* frame_data = frame.raw_samples();
    // size_t num_channels = (size_t)file_info_.channels;
    sf_count_t frame_left = (sf_count_t)frame.num_raw_samples();
    // size_t samples_per_ch = frame.num_raw_samples() / num_channels;

    sf_count_t n_samples = sf_read_float(file_, frame_data, frame_left);
    if (sf_error(file_) != 0) {
        // TODO(gh-183): return error instead of panic
        roc_panic("sndfile source: sf_read_float() failed: %s", sf_strerror(file_));
    }

    if (n_samples == 0) {
        eof_ = true;
    }

    if (n_samples < frame_left && n_samples != 0) {
        memset(frame.raw_samples() + (size_t)n_samples, 0,
               (size_t)(frame_left - n_samples) * sizeof(audio::sample_t));
    }

    return !eof_;
}

bool SndfileSource::seek_(size_t offset) {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: seek: non-open input file");
    }

    roc_log(LogDebug, "sndfile source: resetting position to %lu", (unsigned long)offset);

    sf_count_t err = sf_seek(file_, (sf_count_t)offset, SEEK_SET);
    if (err == -1) {
        roc_log(LogError, "sndfile source: sf_seek(): %s", sf_strerror(file_));
        return false;
    }

    return true;
}

bool SndfileSource::open_(const char* path) {
    if (file_) {
        roc_panic("sndfile source: can't open: already opened");
    }

    file_info_.format = 0;

    file_ = sf_open(path, SFM_READ, &file_info_);
    if (!file_) {
        roc_log(LogInfo, "sndfile source: can't open: input=%s, %s", !path ? NULL : path,
                sf_strerror(file_));
        return false;
    }

    sample_spec_.set_sample_rate((unsigned long)file_info_.samplerate);

    roc_log(LogInfo,
            "sndfile source:"
            " in_rate=%lu out_rate=%lu"
            " in_ch=%lu out_ch=%lu",
            (unsigned long)file_info_.samplerate, (unsigned long)sample_rate_,
            (unsigned long)file_info_.channels, (unsigned long)0);

    return true;
}

void SndfileSource::close_() {
    if (!file_) {
        return;
    }

    roc_log(LogInfo, "sndfile source: closing input");
    int err = sf_close(file_);
    if (err != 0) {
        roc_panic("sndfile source: sf_close() failed. Cannot close input: %s",
                  sf_error_number(err));
    }

    file_ = NULL;
}

} // namespace sndio
} // namespace roc
