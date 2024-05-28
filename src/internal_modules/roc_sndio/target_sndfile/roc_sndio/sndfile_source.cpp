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

SndfileSource::SndfileSource(audio::FrameFactory& frame_factory,
                             core::IArena& arena,
                             const Config& config)
    : frame_factory_(frame_factory)
    , file_(NULL)
    , path_(arena)
    , init_status_(status::NoStatus) {
    if (config.latency != 0) {
        roc_log(LogError,
                "sndfile source: setting io latency not supported by sndfile backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (!config.sample_spec.is_empty()) {
        roc_log(LogError, "sndfile source: setting io encoding not supported");
        init_status_ = status::StatusBadConfig;
        return;
    }

    memset(&file_info_, 0, sizeof(file_info_));

    init_status_ = status::StatusOK;
}

SndfileSource::~SndfileSource() {
    close_();
}

status::StatusCode SndfileSource::init_status() const {
    return init_status_;
}

status::StatusCode SndfileSource::open(const char* driver, const char* path) {
    roc_log(LogDebug, "sndfile source: opening: driver=%s path=%s", driver, path);

    if (file_) {
        roc_panic("sndfile source: can't call open() more than once");
    }

    if (!path) {
        roc_panic("sndfile sink: path is null");
    }

    if (!path_.assign(path)) {
        roc_log(LogError, "sndfile source: can't allocate string");
        return status::StatusNoMem;
    }

    return open_();
}

DeviceType SndfileSource::type() const {
    return DeviceType_Source;
}

ISink* SndfileSource::to_sink() {
    return NULL;
}

ISource* SndfileSource::to_source() {
    return this;
}

audio::SampleSpec SndfileSource::sample_spec() const {
    if (!file_) {
        roc_panic("sndfile source: not opened");
    }

    return sample_spec_;
}

bool SndfileSource::has_state() const {
    return false;
}

bool SndfileSource::has_latency() const {
    return false;
}

bool SndfileSource::has_clock() const {
    return false;
}

status::StatusCode SndfileSource::rewind() {
    roc_log(LogDebug, "sndfile source: rewinding");

    if (file_ && file_info_.seekable) {
        return seek_(0);
    }

    if (file_) {
        close_();
    }
    return open_();
}

void SndfileSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode SndfileSource::read(audio::Frame& frame,
                                       packet::stream_timestamp_t duration) {
    if (!file_) {
        roc_panic("sndfile source: not opened");
    }

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    frame.set_raw(true);

    audio::sample_t* frame_data = frame.raw_samples();
    sf_count_t frame_size = (sf_count_t)frame.num_raw_samples();

    const sf_count_t n_samples = sf_read_float(file_, frame_data, frame_size);
    if (sf_error(file_) != 0) {
        roc_log(LogError, "sndfile source: sf_read_float() failed: %s",
                sf_strerror(file_));
        return status::StatusErrFile;
    }

    if (n_samples == 0) {
        return status::StatusEnd;
    }

    frame.set_num_raw_samples((size_t)n_samples);
    frame.set_duration((size_t)n_samples / sample_spec_.num_channels());

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode SndfileSource::seek_(size_t offset) {
    if (!file_) {
        roc_panic("sndfile source: can't seek: not opened");
    }

    roc_log(LogDebug, "sndfile source: resetting position to %lu", (unsigned long)offset);

    const sf_count_t err = sf_seek(file_, (sf_count_t)offset, SEEK_SET);
    if (err == -1) {
        roc_log(LogError, "sndfile source: sf_seek(): %s", sf_strerror(file_));
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

status::StatusCode SndfileSource::open_() {
    if (file_) {
        roc_panic("sndfile source: can't open: already opened");
    }

    file_info_.format = 0;

    file_ = sf_open(path_.c_str(), SFM_READ, &file_info_);
    if (!file_) {
        roc_log(LogInfo, "sndfile source: can't open: input=%s, %s", path_.c_str(),
                sf_strerror(file_));
        return status::StatusErrFile;
    }

    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);
    sample_spec_.set_sample_rate((size_t)file_info_.samplerate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count((size_t)file_info_.channels);

    roc_log(LogInfo, "sndfile source: opened: %s",
            audio::sample_spec_to_str(sample_spec_).c_str());

    return status::StatusOK;
}

void SndfileSource::close_() {
    if (!file_) {
        return;
    }

    roc_log(LogInfo, "sndfile source: closing input");

    const int err = sf_close(file_);
    if (err != 0) {
        roc_log(LogError,
                "sndfile source: sf_close() failed, cannot properly close input: %s",
                sf_error_number(err));
    }

    file_ = NULL;
}

} // namespace sndio
} // namespace roc
