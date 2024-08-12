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
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

WavSource::WavSource(audio::FrameFactory& frame_factory,
                     core::IArena& arena,
                     const IoConfig& io_config)
    : frame_factory_(frame_factory)
    , file_opened_(false)
    , eof_(false)
    , init_status_(status::NoStatus) {
    if (io_config.latency != 0) {
        roc_log(LogError, "wav source: setting io latency not supported by backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (!io_config.sample_spec.is_empty()) {
        roc_log(LogError, "wav source: setting io encoding not supported by backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    init_status_ = status::StatusOK;
}

WavSource::~WavSource() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "wav source: close failed: status=%s",
                status::code_to_str(code));
    }
}

status::StatusCode WavSource::init_status() const {
    return init_status_;
}

status::StatusCode WavSource::open(const char* path) {
    return open_(path);
}

status::StatusCode WavSource::close() {
    return close_();
}

DeviceType WavSource::type() const {
    return DeviceType_Source;
}

ISink* WavSource::to_sink() {
    return NULL;
}

ISource* WavSource::to_source() {
    return this;
}

audio::SampleSpec WavSource::sample_spec() const {
    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    return sample_spec_;
}

bool WavSource::has_state() const {
    return false;
}

bool WavSource::has_latency() const {
    return false;
}

bool WavSource::has_clock() const {
    return false;
}

status::StatusCode WavSource::rewind() {
    roc_log(LogDebug, "wav source: rewinding");

    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    if (!drwav_seek_to_pcm_frame(&wav_, 0)) {
        roc_log(LogError, "wav source: seek failed");
        return status::StatusErrFile;
    }

    eof_ = false;

    return status::StatusOK;
}

void WavSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode WavSource::read(audio::Frame& frame,
                                   packet::stream_timestamp_t duration,
                                   audio::FrameReadMode mode) {
    if (!file_opened_) {
        roc_panic("wav source: not opened");
    }

    if (eof_) {
        return status::StatusFinish;
    }

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    frame.set_raw(true);

    audio::sample_t* frame_data = frame.raw_samples();
    size_t frame_left = frame.num_raw_samples();
    size_t frame_size = 0;

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
        frame_size += n_samples;
    }

    if (frame_size == 0) {
        return status::StatusFinish;
    }

    frame.set_num_raw_samples(frame_size);
    frame.set_duration(frame_size / sample_spec_.num_channels());

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode WavSource::open_(const char* path) {
    if (file_opened_) {
        roc_panic("wav source: already opened");
    }

    if (!drwav_init_file(&wav_, path, NULL)) {
        roc_log(LogDebug, "wav sink: can't open input file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    roc_log(LogInfo,
            "wav source: opened input file:"
            " path=%s in_bits=%lu in_rate=%lu in_ch=%lu",
            path, (unsigned long)wav_.bitsPerSample, (unsigned long)wav_.sampleRate,
            (unsigned long)wav_.channels);

    sample_spec_.set_sample_rate((size_t)wav_.sampleRate);
    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count((size_t)wav_.channels);

    file_opened_ = true;

    return status::StatusOK;
}

status::StatusCode WavSource::close_() {
    if (!file_opened_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "sndfile source: closing input file");

    file_opened_ = false;

    if (drwav_uninit(&wav_) != DRWAV_SUCCESS) {
        roc_log(LogError, "wav source: can't properly close input file");
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
