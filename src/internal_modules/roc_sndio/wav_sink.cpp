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
#include "roc_core/endian_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

WavSink::WavSink(audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 const Config& config)
    : output_file_(NULL)
    , init_status_(status::NoStatus) {
    if (config.latency != 0) {
        roc_log(LogError, "wav sink: setting io latency not supported");
        init_status_ = status::StatusBadConfig;
        return;
    }

    sample_spec_ = config.sample_spec;

    sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                              44100);

    if (!sample_spec_.is_raw()) {
        roc_log(LogError, "wav sink: sample format can be only \"-\" or \"%s\"",
                audio::pcm_format_to_str(audio::Sample_RawFormat));
        init_status_ = status::StatusBadConfig;
        return;
    }

    header_.reset(new (header_)
                      WavHeader(sample_spec_.num_channels(), sample_spec_.sample_rate(),
                                sizeof(audio::sample_t) * 8));

    init_status_ = status::StatusOK;
}

WavSink::~WavSink() {
    if (output_file_) {
        roc_panic("wav sink: output file is not closed");
    }
}

status::StatusCode WavSink::init_status() const {
    return init_status_;
}

status::StatusCode WavSink::open(const char* path) {
    return open_(path);
}

status::StatusCode WavSink::close() {
    return close_();
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

    const audio::sample_t* samples = frame.raw_samples();
    size_t n_samples = frame.num_raw_samples();

    if (n_samples > 0) {
        if (fseek(output_file_, 0, SEEK_SET) != 0) {
            roc_log(LogError, "wav sink: failed to seek to the beginning of the file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }

        const WavHeader::WavHeaderData& wav_header =
            header_->update_and_get_header(n_samples);
        if (fwrite(&wav_header, sizeof(wav_header), 1, output_file_) != 1) {
            roc_log(LogError, "wav sink: failed to write header: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }

        if (fseek(output_file_, 0, SEEK_END) != 0) {
            roc_log(LogError,
                    "wav sink: failed to seek to append position of the file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }

        if (fwrite(samples, sizeof(audio::sample_t), n_samples, output_file_)
            != n_samples) {
            roc_log(LogError, "wav sink: failed to write samples: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }

        if (fflush(output_file_) != 0) {
            roc_log(LogError, "wav sink: failed to flush data to the file: %s",
                    core::errno_to_str(errno).c_str());
            return status::StatusErrFile;
        }
    }

    return status::StatusOK;
}

status::StatusCode WavSink::flush() {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    return status::StatusOK;
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
