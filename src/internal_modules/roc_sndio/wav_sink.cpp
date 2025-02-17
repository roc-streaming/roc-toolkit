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

WavSink::WavSink(core::IArena& arena, const Config& config)
    : output_file_(NULL)
    , valid_(false) {
    if (config.latency != 0) {
        roc_log(LogError, "wav sink: setting io latency not supported");
        return;
    }

    sample_spec_ = config.sample_spec;

    sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                              44100);

    if (!sample_spec_.is_raw()) {
        roc_log(LogError, "wav sink: sample format can be only \"-\" or \"%s\"",
                audio::pcm_format_to_str(audio::Sample_RawFormat));
        return;
    }

    header_.reset(new (header_)
                      WavHeader(sample_spec_.num_channels(), sample_spec_.sample_rate(),
                                sizeof(audio::sample_t) * 8));

    valid_ = true;
}

WavSink::~WavSink() {
    close_();
}

bool WavSink::is_valid() const {
    return valid_;
}

bool WavSink::open(const char* path) {
    roc_panic_if(!valid_);

    if (!open_(path)) {
        return false;
    }

    return true;
}

ISink* WavSink::to_sink() {
    return this;
}

ISource* WavSink::to_source() {
    return NULL;
}

DeviceType WavSink::type() const {
    return DeviceType_Sink;
}

DeviceState WavSink::state() const {
    return DeviceState_Active;
}

void WavSink::pause() {
    // no-op
}

bool WavSink::resume() {
    return true;
}

bool WavSink::restart() {
    return true;
}

audio::SampleSpec WavSink::sample_spec() const {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    return sample_spec_;
}

core::nanoseconds_t WavSink::latency() const {
    return 0;
}

bool WavSink::has_latency() const {
    return false;
}

bool WavSink::has_clock() const {
    return false;
}

void WavSink::write(audio::Frame& frame) {
    if (!output_file_) {
        roc_panic("wav sink: not opened");
    }

    const audio::sample_t* samples = frame.raw_samples();
    size_t n_samples = frame.num_raw_samples();

    if (n_samples > 0) {
        if (fseek(output_file_, 0, SEEK_SET)) {
            roc_log(LogError, "wav sink: failed to seek to the beginning of the file: %s",
                    core::errno_to_str(errno).c_str());
        }

        const WavHeader::WavHeaderData& wav_header =
            header_->update_and_get_header(n_samples);
        if (fwrite(&wav_header, sizeof(wav_header), 1, output_file_) != 1) {
            roc_log(LogError, "wav sink: failed to write header: %s",
                    core::errno_to_str(errno).c_str());
        }

        if (fseek(output_file_, 0, SEEK_END)) {
            roc_log(LogError,
                    "wav sink: failed to seek to append position of the file: %s",
                    core::errno_to_str(errno).c_str());
        }

        if (fwrite(samples, sizeof(audio::sample_t), n_samples, output_file_)
            != n_samples) {
            roc_log(LogError, "wav sink: failed to write samples: %s",
                    core::errno_to_str(errno).c_str());
        }

        if (fflush(output_file_)) {
            roc_log(LogError, "wav sink: failed to flush data to the file: %s",
                    core::errno_to_str(errno).c_str());
        }
    }
}

bool WavSink::open_(const char* path) {
    if (output_file_) {
        roc_panic("wav sink: already opened");
    }

    output_file_ = fopen(path, "w");
    if (!output_file_) {
        roc_log(LogDebug, "wav sink: can't open output file: %s",
                core::errno_to_str(errno).c_str());
        return false;
    }

    if (*path == '-') {
        roc_log(LogDebug, "wav sink: output file to stdout");
        output_file_ = stdout;
    }

    roc_log(LogInfo,
            "wav sink: opened output file:"
            " path=%s out_bits=%lu out_rate=%lu out_ch=%lu",
            path, (unsigned long)header_->bits_per_sample(),
            (unsigned long)header_->sample_rate(),
            (unsigned long)header_->num_channels());

    return true;
}

void WavSink::close_() {
    if (!output_file_) {
        return;
    }

    roc_log(LogDebug, "wav sink: closing output file");

    if (fclose(output_file_)) {
        roc_panic("wav sink: can't close output file: %s",
                  core::errno_to_str(errno).c_str());
    }

    output_file_ = NULL;
}

} // namespace sndio
} // namespace roc
