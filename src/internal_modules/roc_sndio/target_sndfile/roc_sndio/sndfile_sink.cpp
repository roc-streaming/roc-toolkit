/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_sink.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/sndfile_helpers.h"
#include "roc_sndio/sndfile_tables.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SndfileSink::SndfileSink(audio::FrameFactory& frame_factory,
                         core::IArena& arena,
                         const IoConfig& io_config,
                         const char* path)
    : IDevice(arena)
    , ISink(arena)
    , file_(NULL)
    , init_status_(status::NoStatus) {
    file_spec_ = io_config.sample_spec;
    file_spec_.use_defaults(audio::Format_Invalid, audio::PcmSubformat_Invalid,
                            audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                            audio::ChanMask_Surround_Stereo, 44100);

    memset(&file_info_, 0, sizeof(file_info_));

    if ((init_status_ = open_(path)) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

SndfileSink::~SndfileSink() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "sndfile sink: close failed: status=%s",
                status::code_to_str(code));
    }
}

status::StatusCode SndfileSink::init_status() const {
    return init_status_;
}

DeviceType SndfileSink::type() const {
    return DeviceType_Sink;
}

ISink* SndfileSink::to_sink() {
    return this;
}

ISource* SndfileSink::to_source() {
    return NULL;
}

audio::SampleSpec SndfileSink::sample_spec() const {
    if (!file_) {
        roc_panic("sndfile sink: not opened");
    }

    return frame_spec_;
}

core::nanoseconds_t SndfileSink::frame_length() const {
    return 0;
}

bool SndfileSink::has_state() const {
    return false;
}

bool SndfileSink::has_latency() const {
    return false;
}

bool SndfileSink::has_clock() const {
    return false;
}

status::StatusCode SndfileSink::write(audio::Frame& frame) {
    if (!file_) {
        roc_panic("sndfile sink: not opened");
    }

    frame_spec_.validate_frame(frame);

    audio::sample_t* frame_data = frame.raw_samples();
    sf_count_t frame_size = (sf_count_t)frame.num_raw_samples();

    // Write entire float buffer in one call
    const sf_count_t count = sf_write_float(file_, frame_data, frame_size);
    const int err = sf_error(file_);

    if (count != frame_size || err != 0) {
        roc_log(LogError, "sndfile source: sf_write_float() failed: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

status::StatusCode SndfileSink::flush() {
    if (!file_) {
        roc_panic("sndfile sink: not opened");
    }

    return status::StatusOK;
}

status::StatusCode SndfileSink::close() {
    return close_();
}

void SndfileSink::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode SndfileSink::open_(const char* path) {
    roc_log(LogDebug, "sndfile sink: opening: path=%s", path);

    file_info_.samplerate = (int)file_spec_.sample_rate();
    file_info_.channels = (int)file_spec_.num_channels();

    status::StatusCode code = status::NoStatus;

    if ((code = sndfile_select_major_format(file_info_, file_spec_, path))
        != status::StatusOK) {
        return code;
    }

    if ((code = sndfile_select_sub_format(file_info_, file_spec_, path))
        != status::StatusOK) {
        return code;
    }

    if (!(file_ = sf_open(path, SFM_WRITE, &file_info_))) {
        roc_log(LogError, "sndfile sink: can't open output file: %s",
                sf_error_number(sf_error(NULL)));
        return status::StatusErrFile;
    }

    if (!sf_command(file_, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE)) {
        roc_log(LogError, "sndfile sink: can't open output file: %s", sf_strerror(file_));
        return status::StatusErrFile;
    }

    if (!file_spec_.has_format() || !file_spec_.has_subformat()) {
        if ((code = sndfile_detect_format(file_info_, file_spec_)) != status::StatusOK) {
            return code;
        }
    }

    file_spec_.set_sample_rate((size_t)file_info_.samplerate);
    file_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    file_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    file_spec_.channel_set().set_count((size_t)file_info_.channels);

    frame_spec_ = file_spec_;
    frame_spec_.set_format(audio::Format_Pcm);
    frame_spec_.set_pcm_subformat(audio::PcmSubformat_Raw);

    roc_log(LogInfo, "sndfile sink: opened output file: %s",
            audio::sample_spec_to_str(file_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode SndfileSink::close_() {
    if (!file_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "sndfile sink: closing output file");

    const int err = sf_close(file_);
    file_ = NULL;

    if (err != 0) {
        roc_log(LogError, "sndfile sink: can't properly close output file: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
