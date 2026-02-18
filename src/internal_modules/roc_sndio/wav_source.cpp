/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/wav_source.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

size_t file_read(void* file, void* buf, size_t bufsz) {
    return fread(buf, 1, bufsz, (FILE*)file);
}

drwav_bool32 file_seek(void* file, int offset, drwav_seek_origin origin) {
    return fseek((FILE*)file, offset,
                 origin == drwav_seek_origin_current ? SEEK_CUR : SEEK_SET)
        == 0;
}

} // namespace

WavSource::WavSource(audio::FrameFactory& frame_factory,
                     core::IArena& arena,
                     const IoConfig& io_config,
                     const char* path)
    : IDevice(arena)
    , ISource(arena)
    , frame_factory_(frame_factory)
    , input_file_(NULL)
    , eof_(false)
    , init_status_(status::NoStatus) {
    if (io_config.sample_spec.has_format()) {
        if (io_config.sample_spec.format() != audio::Format_Wav) {
            roc_log(LogDebug,
                    "wav source: requested format '%s' not supported by backend: spec=%s",
                    io_config.sample_spec.format_name(),
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            // Not a wav file, go to next backend.
            init_status_ = status::StatusNoFormat;
            return;
        }
    }

    if (io_config.sample_spec.has_subformat() || io_config.sample_spec.has_sample_rate()
        || io_config.sample_spec.has_channel_set()) {
        roc_log(LogError,
                "wav source: invalid io encoding: <subformat>, <rate> and <channels>"
                " not allowed for input file when <format> is 'wav', set them to \"-\"");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if ((init_status_ = open_(path)) != status::StatusOK) {
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
    if (!input_file_) {
        roc_panic("wav source: not opened");
    }

    return sample_spec_;
}

core::nanoseconds_t WavSource::frame_length() const {
    return 0;
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

    if (!input_file_) {
        roc_panic("wav source: not opened");
    }

    if (!drwav_seek_to_pcm_frame(&wav_decoder_, 0)) {
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
    if (!input_file_) {
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

        n_samples = drwav_read_pcm_frames_f32(
                        &wav_decoder_, n_samples / wav_decoder_.channels, frame_data)
            * wav_decoder_.channels;

        if (ferror(input_file_)) {
            roc_log(LogError, "wav source: can't read input file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }

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
    frame.set_duration(
        packet::stream_timestamp_t(frame_size / sample_spec_.num_channels()));

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode WavSource::close() {
    return close_();
}

void WavSource::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode WavSource::open_(const char* path) {
    roc_log(LogDebug, "wav source: opening: path=%s", path);

    if (strcmp(path, "-") == 0) {
        input_file_ = stdin;
    } else {
        if (!(input_file_ = fopen(path, "rb"))) {
            roc_log(LogError, "wav source: can't open input file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
    }

    if (!drwav_init(&wav_decoder_, &file_read, &file_seek, input_file_, NULL)) {
        roc_log(LogDebug, "wav source: can't recognize input file format");
        if (input_file_ != stdin) {
            fclose(input_file_);
        }
        input_file_ = NULL;
        return status::StatusNoFormat;
    }

    sample_spec_.set_format(audio::Format_Pcm);
    sample_spec_.set_pcm_subformat(audio::PcmSubformat_Raw);
    sample_spec_.set_sample_rate((size_t)wav_decoder_.sampleRate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count((size_t)wav_decoder_.channels);

    roc_log(LogInfo, "wav source: opened input file: %s",
            audio::sample_spec_to_str(sample_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode WavSource::close_() {
    if (!input_file_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "wav source: closing input file");

    if (drwav_uninit(&wav_decoder_) != DRWAV_SUCCESS) {
        roc_log(LogError, "wav source: can't properly close input file");
        return status::StatusErrFile;
    }

    if (input_file_ == stdin) {
        input_file_ = NULL;
    } else {
        const int err = fclose(input_file_);
        input_file_ = NULL;

        if (err != 0) {
            roc_log(LogError, "wav source: can't properly close input file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
