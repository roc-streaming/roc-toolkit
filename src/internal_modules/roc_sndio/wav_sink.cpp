/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/wav_sink.h"
#include "roc_audio/format.h"
#include "roc_audio/pcm_subformat.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/endian_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

bool has_extension(const char* path, const char* ext) {
    size_t path_len = strlen(path);
    size_t ext_len = strlen(ext);
    if (ext_len > path_len) {
        return false;
    }
    return strncmp(path + path_len - ext_len, ext, ext_len) == 0;
}

} // namespace

WavSink::WavSink(audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 const IoConfig& io_config,
                 const char* path)
    : IDevice(arena)
    , ISink(arena)
    , output_file_(NULL)
    , is_first_(true)
    , init_status_(status::NoStatus) {
    if (io_config.sample_spec.has_format()) {
        if (io_config.sample_spec.format() != audio::Format_Wav) {
            roc_log(LogDebug,
                    "wav sink: requested format '%s' not supported by backend: spec=%s",
                    io_config.sample_spec.format_name(),
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            // Not a wav file, go to next backend.
            init_status_ = status::StatusNoFormat;
            return;
        }
    } else {
        if (!has_extension(path, ".wav")) {
            roc_log(
                LogDebug,
                "wav sink: requested file extension not supported by backend: path=%s",
                path);
            // Not a wav file, go to next backend.
            init_status_ = status::StatusNoFormat;
            return;
        }
    }

    if (io_config.sample_spec.has_subformat()) {
        if (io_config.sample_spec.pcm_subformat() == audio::PcmSubformat_Invalid) {
            roc_log(LogError,
                    "wav sink: invalid io encoding:"
                    " <subformat> '%s' not allowed when <format> is 'wav':"
                    " <subformat> must be pcm (like s16 or f32)",
                    io_config.sample_spec.subformat_name());
            init_status_ = status::StatusBadConfig;
            return;
        }

        const audio::PcmTraits subfmt =
            audio::pcm_subformat_traits(io_config.sample_spec.pcm_subformat());

        if (!subfmt.has_flags(audio::Pcm_IsSigned)) {
            roc_log(LogError,
                    "wav sink: invalid io encoding:"
                    " <subformat> '%s' not allowed when <format> is 'wav':"
                    " must be float (like f32) or signed integer (like s16)",
                    io_config.sample_spec.subformat_name());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (!subfmt.has_flags(audio::Pcm_IsPacked | audio::Pcm_IsAligned)) {
            roc_log(LogError,
                    "wav sink: invalid io encoding:"
                    " <subformat> '%s' not allowed when <format> is 'wav':"
                    " must be packed (like s24, not s24_4) and byte-aligned"
                    " (like s16, not s18)",
                    io_config.sample_spec.subformat_name());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (io_config.sample_spec.pcm_subformat() != subfmt.default_variant
            && io_config.sample_spec.pcm_subformat() != subfmt.le_variant) {
            roc_log(LogError,
                    "wav sink: invalid io encoding:"
                    " <subformat> '%s' not allowed when <format> is 'wav':"
                    " must be default-endian (like s16) or little-endian (like s16_le)",
                    io_config.sample_spec.subformat_name());
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    file_spec_ = io_config.sample_spec;
    file_spec_.use_defaults(audio::Format_Wav, audio::PcmSubformat_Raw,
                            audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                            audio::ChanMask_Surround_Stereo, 44100);

    const audio::PcmTraits subfmt =
        audio::pcm_subformat_traits(file_spec_.pcm_subformat());

    frame_spec_ = file_spec_;
    frame_spec_.set_format(audio::Format_Pcm);
    if (frame_spec_.pcm_subformat() == subfmt.default_variant) {
        frame_spec_.set_pcm_subformat(subfmt.le_variant);
    }

    const uint16_t fmt_code =
        subfmt.has_flags(audio::Pcm_IsInteger) ? WAV_FORMAT_PCM : WAV_FORMAT_IEEE_FLOAT;

    header_.reset(new (header_) WavHeader(
        fmt_code, subfmt.bit_width, file_spec_.sample_rate(), file_spec_.num_channels()));

    if ((init_status_ = open_(path)) != status::StatusOK) {
        return;
    }

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

    return frame_spec_;
}

core::nanoseconds_t WavSink::frame_length() const {
    return 0;
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

    frame_spec_.validate_frame(frame);

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
    roc_log(LogDebug, "wav sink: opening: path=%s", path);

    if (strcmp(path, "-") == 0) {
        output_file_ = stdout;
    } else {
        if (!(output_file_ = fopen(path, "wb"))) {
            roc_log(LogDebug, "wav sink: can't open output file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
    }

    roc_log(LogInfo, "wav sink: opened output file: %s",
            audio::sample_spec_to_str(file_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode WavSink::close_() {
    if (!output_file_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "wav sink: closing output file");

    if (output_file_ == stdout) {
        output_file_ = NULL;
    } else {
        const int err = fclose(output_file_);
        output_file_ = NULL;

        if (err != 0) {
            roc_log(LogError, "wav sink: can't properly close output file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
