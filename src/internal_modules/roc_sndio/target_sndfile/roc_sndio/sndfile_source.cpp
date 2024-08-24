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
#include "roc_sndio/sndfile_helpers.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SndfileSource::SndfileSource(audio::FrameFactory& frame_factory,
                             core::IArena& arena,
                             const IoConfig& io_config,
                             const char* path)
    : IDevice(arena)
    , ISource(arena)
    , frame_factory_(frame_factory)
    , file_(NULL)
    , path_(arena)
    , init_status_(status::NoStatus) {
    requested_spec_ = io_config.sample_spec;

    memset(&file_info_, 0, sizeof(file_info_));

    if (!path_.assign(path)) {
        roc_log(LogError, "sndfile source: can't allocate string");
        init_status_ = status::StatusNoMem;
        return;
    }

    if ((init_status_ = open_()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

SndfileSource::~SndfileSource() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "sndfile source: close failed: status=%s",
                status::code_to_str(code));
    }
}

status::StatusCode SndfileSource::init_status() const {
    return init_status_;
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

    return frame_spec_;
}

core::nanoseconds_t SndfileSource::frame_length() const {
    return 0;
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
    if (file_ && file_info_.seekable) {
        roc_log(LogDebug, "sndfile source: rewinding");
        return seek_(0);
    }

    roc_log(LogDebug, "sndfile source: reopening");

    if (file_) {
        const status::StatusCode close_code = close_();
        if (close_code != status::StatusOK) {
            return close_code;
        }
    }

    return open_();
}

void SndfileSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode SndfileSource::read(audio::Frame& frame,
                                       packet::stream_timestamp_t duration,
                                       audio::FrameReadMode mode) {
    if (!file_) {
        roc_panic("sndfile source: not opened");
    }

    if (!frame_factory_.reallocate_frame(
            frame, frame_spec_.stream_timestamp_2_bytes(duration))) {
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
        roc_log(LogDebug, "sndfile source: got eof from input file");
        return status::StatusFinish;
    }

    frame.set_num_raw_samples((size_t)n_samples);
    frame.set_duration((size_t)n_samples / frame_spec_.num_channels());

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode SndfileSource::close() {
    return close_();
}

void SndfileSource::dispose() {
    arena().dispose_object(*this);
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
    roc_log(LogDebug, "sndfile source: opening: path=%s", path_.c_str());

    // Overwrite file spec with originally requested spec
    // (file spec may be non-empty if we're reopening file because of rewind).
    file_spec_ = requested_spec_;

    file_info_.samplerate = (int)file_spec_.sample_rate();
    file_info_.channels = (int)file_spec_.num_channels();

    status::StatusCode code = status::NoStatus;

    if (file_spec_.has_format()) {
        if ((code = sndfile_select_major_format(file_info_, file_spec_, path_.c_str()))
            != status::StatusOK) {
            return code;
        }
    }

    if ((code = sndfile_check_input_spec(file_info_, file_spec_, path_.c_str()))
        != status::StatusOK) {
        return code;
    }

    if (file_spec_.has_subformat()) {
        if ((code = sndfile_select_sub_format(file_info_, file_spec_, path_.c_str()))
            != status::StatusOK) {
            return code;
        }
    }

    const int requested_format = file_info_.format;

    if (!(file_ = sf_open(path_.c_str(), SFM_READ, &file_info_))) {
        const int err = sf_error(NULL);
        if (err == SF_ERR_UNRECOGNISED_FORMAT || err == SF_ERR_UNSUPPORTED_ENCODING) {
            // Try another backend.
            roc_log(LogDebug, "sndfile source: can't recognize input file format");
            return status::StatusNoFormat;
        }
        roc_log(LogError, "sndfile source: can't open input file: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    if ((file_info_.format & requested_format) != requested_format) {
        roc_log(LogError,
                "sndfile source: input file doesn't match requested format '%s'",
                file_spec_.format_name());
        return status::StatusErrFile;
    }

    // Fill file spec.
    if (!file_spec_.has_format() || !file_spec_.has_subformat()) {
        if ((code = sndfile_detect_format(file_info_, file_spec_)) != status::StatusOK) {
            return code;
        }
    }
    file_spec_.set_sample_rate((size_t)file_info_.samplerate);
    file_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    file_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    file_spec_.channel_set().set_count((size_t)file_info_.channels);

    // Fill frame spec.
    frame_spec_ = file_spec_;
    frame_spec_.set_format(audio::Format_Pcm);
    frame_spec_.set_pcm_subformat(audio::PcmSubformat_Raw);

    roc_log(LogInfo, "sndfile source: opened input file: %s",
            audio::sample_spec_to_str(file_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode SndfileSource::close_() {
    if (!file_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "sndfile source: closing input file");

    const int err = sf_close(file_);
    file_ = NULL;

    if (err != 0) {
        roc_log(LogError, "sndfile source: can't properly close input file: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
