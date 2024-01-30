/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/wav_sink.h"
#include "roc_core/endian_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

namespace {

const size_t DefaultChans = 2;
const size_t DefaultRate = 44100;

} // namespace

WavSink::WavSink(core::IArena& arena, const Config& config)
    : output_file_(NULL)
    , header_(config.sample_spec.num_channels() != 0 ? config.sample_spec.num_channels()
                                                     : DefaultChans,
              config.sample_spec.sample_rate() != 0 ? config.sample_spec.sample_rate()
                                                    : DefaultRate,
              sizeof(audio::sample_t) * 8)
    , valid_(false) {
    if (config.latency != 0) {
        roc_log(LogError, "wav sink: setting io latency not supported by wav backend");
        return;
    }

    frame_length_ = config.frame_length;

    if (frame_length_ == 0) {
        roc_log(LogError, "wav sink: frame length is zero");
        return;
    }

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

    roc_log(LogDebug, "wav sink: opening: path=%s", path);

    if (output_file_ != NULL) {
        roc_panic("wav sink: can't call open() more than once");
    }

    if (!open_(path)) {
        return false;
    }

    return true;
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
    roc_panic_if(!valid_);

    if (!output_file_) {
        roc_panic("wav sink: sample_spec(): non-open output file or device");
    }

    audio::ChannelSet channel_set;
    channel_set.set_layout(audio::ChanLayout_Surround);
    channel_set.set_order(audio::ChanOrder_Smpte);
    channel_set.set_channel_range(0, header_.num_channels() - 1, true);

    return audio::SampleSpec(size_t(header_.sample_rate()), audio::Sample_RawFormat,
                             channel_set);
}

core::nanoseconds_t WavSink::latency() const {
    roc_panic_if(!valid_);

    if (!output_file_) {
        roc_panic("wav sink: latency(): non-open output file");
    }

    return 0;
}

bool WavSink::has_latency() const {
    roc_panic_if(!valid_);

    if (!output_file_) {
        roc_panic("wav sink: has_latency(): non-open output file");
    }

    return false;
}

bool WavSink::has_clock() const {
    roc_panic_if(!valid_);

    if (!output_file_) {
        roc_panic("wav sink: has_clock(): non-open output file");
    }

    return false;
}

void WavSink::write(audio::Frame& frame) {
    roc_panic_if(!valid_);

    const audio::sample_t* frame_data = frame.samples();
    size_t frame_size = frame.num_samples();

    write_(frame_data, frame_size);
}

bool WavSink::open_(const char* path) {
    output_file_ = fopen(path, "w");
    if (!output_file_) {
        roc_log(LogDebug, "wav sink: can't open: path=%s, errno=%s", path,
                core::errno_to_str(errno).c_str());
        return false;
    }

    roc_log(LogInfo, "wav sink: opened: out_bits=%lu out_rate=%lu out_ch=%lu",
            (unsigned long)header_.bits_per_sample(),
            (unsigned long)header_.sample_rate(), (unsigned long)header_.num_channels());

    return true;
}

void WavSink::write_(const audio::sample_t* samples, size_t n_samples) {
    if (n_samples > 0) {
        if (fseek(output_file_, 0, SEEK_SET)) {
            roc_log(LogError, "wav sink: failed to seek to the beginning of the file: %s",
                    core::errno_to_str(errno).c_str());
        }

        const WavHeader::WavHeaderData& wav_header =
            header_.update_and_get_header(n_samples);
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

void WavSink::close_() {
    if (!output_file_) {
        return;
    }

    roc_log(LogDebug, "wav sink: closing output");

    if (fclose(output_file_)) {
        roc_panic("wav sink: can't close output");
    }

    output_file_ = NULL;
}

} // namespace sndio
} // namespace roc
