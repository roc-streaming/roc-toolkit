/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_source.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"

namespace roc {
namespace sndio {

SndfileSource::SndfileSource(core::IArena& arena, const Config& config)
    : file_(NULL)
    , path_(arena)
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

    memset(&file_info_, 0, sizeof(file_info_));
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

    roc_log(LogDebug, "sndfile source: opening: driver=%s path=%s", driver, path);

    if (file_) {
        roc_panic("sndfile source: can't call open() more than once");
    }

    if (!path) {
        roc_panic("sndfile sink: path is null");
    }

    if (!path_.assign(path)) {
        roc_log(LogError, "sndfile source: can't allocate string");
        return false;
    }

    if (!open_()) {
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

    if (file_ && file_info_.seekable) {
        if (!seek_(0)) {
            roc_log(LogError, "sndfile source: seek failed when restarting");
            return false;
        }
    } else {
        if (file_) {
            close_();
        }

        if (!open_()) {
            roc_log(LogError, "sndfile source: open failed when restarting");
            return false;
        }
    }

    return true;
}

audio::SampleSpec SndfileSource::sample_spec() const {
    if (!file_) {
        roc_panic("sndfile source: not opened");
    }

    return sample_spec_;
}

core::nanoseconds_t SndfileSource::latency() const {
    return 0;
}

bool SndfileSource::has_latency() const {
    return false;
}

bool SndfileSource::has_clock() const {
    return false;
}

void SndfileSource::reclock(core::nanoseconds_t) {
    // no-op
}

bool SndfileSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (!file_) {
        roc_panic("sndfile source: not opened");
    }

    audio::sample_t* frame_data = frame.raw_samples();
    sf_count_t frame_left = (sf_count_t)frame.num_raw_samples();

    sf_count_t n_samples = sf_read_float(file_, frame_data, frame_left);
    if (sf_error(file_) != 0) {
        // TODO(gh-183): return error instead of panic
        roc_panic("sndfile source: sf_read_float() failed: %s", sf_strerror(file_));
    }

    if (n_samples < frame_left && n_samples != 0) {
        memset(frame.raw_samples() + (size_t)n_samples, 0,
               (size_t)(frame_left - n_samples) * sizeof(audio::sample_t));
    }

    return n_samples != 0;
}

bool SndfileSource::seek_(size_t offset) {
    if (!file_) {
        roc_panic("sndfile source: can't seek: not opened");
    }

    roc_log(LogDebug, "sndfile source: resetting position to %lu", (unsigned long)offset);

    sf_count_t err = sf_seek(file_, (sf_count_t)offset, SEEK_SET);
    if (err == -1) {
        roc_log(LogError, "sndfile source: sf_seek(): %s", sf_strerror(file_));
        return false;
    }

    return true;
}

bool SndfileSource::open_() {
    if (file_) {
        roc_panic("sndfile source: can't open: already opened");
    }

    file_info_.format = 0;

    file_ = sf_open(path_.c_str(), SFM_READ, &file_info_);
    if (!file_) {
        roc_log(LogInfo, "sndfile source: can't open: input=%s, %s", path_.c_str(),
                sf_strerror(file_));
        return false;
    }

    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);
    sample_spec_.set_sample_rate((size_t)file_info_.samplerate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_range(0, (size_t)file_info_.channels - 1);

    roc_log(LogInfo, "sndfile source: opened: %s",
            audio::sample_spec_to_str(sample_spec_).c_str());

    return true;
}

void SndfileSource::close_() {
    if (!file_) {
        return;
    }

    roc_log(LogInfo, "sndfile source: closing input");

    int err = sf_close(file_);
    if (err != 0) {
        roc_log(LogError,
                "sndfile source: sf_close() failed, cannot properly close input: %s",
                sf_error_number(err));
    }

    file_ = NULL;
}

} // namespace sndio
} // namespace roc
