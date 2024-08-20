/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/wav_sink.h"
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample_format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/endian_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

WavSink::WavSink(audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 const IoConfig& io_config)
    : IDevice(arena)
    , ISink(arena)
    , output_file_(NULL)
    , is_first_(true)
    , init_status_(status::NoStatus) {
    if (io_config.latency != 0) {
        roc_log(LogError, "wav sink: setting io latency not supported by backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    sample_spec_ = io_config.sample_spec;

    sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                              44100);

    if (!sample_spec_.is_pcm()) {
        roc_log(LogError, "wav sink: unsupported format: must be pcm: spec=%s",
                audio::sample_spec_to_str(sample_spec_).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    const audio::PcmTraits fmt = audio::pcm_format_traits(sample_spec_.pcm_format());

    if (!fmt.has_flags(audio::Pcm_IsSigned)) {
        roc_log(LogError, "wav sink: unsupported format: must be signed: spec=%s",
                audio::sample_spec_to_str(sample_spec_).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (!fmt.has_flags(audio::Pcm_IsPacked | audio::Pcm_IsAligned)) {
        roc_log(LogError,
                "wav sink: unsupported format: must be packed and byte-aligned: spec=%s",
                audio::sample_spec_to_str(sample_spec_).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    // WAV format is always little-endian.
    if (sample_spec_.pcm_format() != fmt.default_variant
        && sample_spec_.pcm_format() != fmt.le_variant) {
        roc_log(LogError,
                "wav sink: sample format must be default-endian (like s16) or"
                " little-endian (like s16_le): spec=%s",
                audio::sample_spec_to_str(sample_spec_).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (sample_spec_.pcm_format() == fmt.default_variant) {
        sample_spec_.set_pcm_format(fmt.le_variant);
    }

    const uint16_t fmt_code =
        fmt.has_flags(audio::Pcm_IsInteger) ? WAV_FORMAT_PCM : WAV_FORMAT_IEEE_FLOAT;

    header_.reset(new (header_)
                      WavHeader(fmt_code, fmt.bit_width, sample_spec_.sample_rate(),
                                sample_spec_.num_channels()));

    init_status_ = status::StatusOK;
}

WavSink::~WavSink() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "wav sink: close failed: status=%s", status::code_to_str(code));
    }
}

status::StatusCode WavSink::init_status() const {
    return init_status_;
}

status::StatusCode WavSink::open(const char* path) {
    return open_(path);
}

DeviceType WavSink::type() const {
    return DeviceType_Sink;
}

ISink* WavSink::to_sink() {
    return this;
}

ISource* WavSink::to_source() {
    return NULL;
}

audio::SampleSpec WavSink::sample_spec() const {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    return sample_spec_;
}

bool WavSink::has_state() const {
    return false;
}

bool WavSink::has_latency() const {
    return false;
}

bool WavSink::has_clock() const {
    return false;
}

status::StatusCode WavSink::write(audio::Frame& frame) {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    if (is_first_) {
        const WavHeader::WavHeaderData& wav_header = header_->update_and_get_header(0);
        if (fwrite(&wav_header, sizeof(wav_header), 1, output_file_) != 1) {
            roc_log(LogError, "wav sink: failed to write header: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
        is_first_ = false;
    }

    // First append samples to file.
    if (fwrite(frame.bytes(), 1, frame.num_bytes(), output_file_) != frame.num_bytes()) {
        roc_log(LogError, "wav sink: failed to write samples: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    if (fseek(output_file_, 0, SEEK_SET) != 0) {
        roc_log(LogError, "wav sink: failed to seek to the beginning of file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    // Then update header so that someone who is reading the file concurrently
    // could process the appended samples.
    const WavHeader::WavHeaderData& wav_header =
        header_->update_and_get_header(frame.duration());
    if (fwrite(&wav_header, sizeof(wav_header), 1, output_file_) != 1) {
        roc_log(LogError, "wav sink: failed to write header: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    if (fseek(output_file_, 0, SEEK_END) != 0) {
        roc_log(LogError, "wav sink: failed to seek to the end of file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    if (fflush(output_file_) != 0) {
        roc_log(LogError, "wav sink: failed to flush data to the file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

status::StatusCode WavSink::flush() {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    return status::StatusOK;
}

status::StatusCode WavSink::close() {
    return close_();
}

void WavSink::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode WavSink::open_(const char* path) {
    if (output_file_) {
        roc_panic("wav sink: already opened");
    }

    output_file_ = fopen(path, "w");

    if (!output_file_) {
        roc_log(LogDebug, "wav sink: can't open output file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    roc_log(LogInfo,
            "wav sink: opened output file:"
            " path=%s out_bits=%lu out_rate=%lu out_ch=%lu",
            path, (unsigned long)header_->bits_per_sample(),
            (unsigned long)header_->sample_rate(),
            (unsigned long)header_->num_channels());

    return status::StatusOK;
}

status::StatusCode WavSink::close_() {
    if (!output_file_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "wav sink: closing output file");

    const int err = fclose(output_file_);
    output_file_ = NULL;

    if (err != 0) {
        roc_log(LogError, "wav sink: can't properly close output file: %s",
                core::errno_to_str(errno).c_str());
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
