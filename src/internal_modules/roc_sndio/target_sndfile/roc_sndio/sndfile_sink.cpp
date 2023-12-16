/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_sink.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"

namespace roc {
namespace sndio {
namespace {

struct FileMap {
    const char* format_cstring;
    int format_enum;
} file_type_map[] = {
    { "aiff", SF_FORMAT_AIFF },   { "au", SF_FORMAT_AU },     { "avr", SF_FORMAT_AVR },
    { "caf", SF_FORMAT_CAF },     { "htk", SF_FORMAT_HTK },   { "iff", SF_FORMAT_SVX },
    { "mat", SF_FORMAT_MAT4 },    { "mat4", SF_FORMAT_MAT4 }, { "mat5", SF_FORMAT_MAT5 },
    { "mpc", SF_FORMAT_MPC2K },   { "paf", SF_FORMAT_PAF },   { "pvf", SF_FORMAT_PVF },
    { "raw", SF_FORMAT_RAW },     { "rf64", SF_FORMAT_RF64 }, { "sd2", SF_FORMAT_SD2 },
    { "sds", SF_FORMAT_SDS },     { "sf", SF_FORMAT_IRCAM },  { "voc", SF_FORMAT_VOC },
    { "w64", SF_FORMAT_W64 },     { "wav", SF_FORMAT_WAV },   { "nist", SF_FORMAT_NIST },
    { "wavex", SF_FORMAT_WAVEX }, { "wve", SF_FORMAT_WVE },   { "xi", SF_FORMAT_XI },
};

bool detect_file_extension(const char** driver, const char* path) {
    const char* dot = strrchr(path, '.');

    if (!dot || dot == path) {
        return false;
    }

    const char* file_extension = dot + 1;
    bool found_extension = false;

    for (size_t sndfile_extension = 0; sndfile_extension < ROC_ARRAY_SIZE(file_type_map);
         sndfile_extension++) {
        if (strcmp(file_type_map[sndfile_extension].format_cstring, file_extension)
            == 0) {
            found_extension = true;
        }
    }

    if (!found_extension) {
        return false;
    }

    roc_log(LogDebug, "detected file format type `%s'", file_extension);
    *driver = file_extension;
    return true;
}

bool map_to_sndfile(SF_INFO& sfinfo, const char* driver, int& bits) {
    int format = 0;
    for (size_t format_struct_index = 0;
         format_struct_index < sizeof(file_type_map) / sizeof(file_type_map[0]);
         format_struct_index++) {
        if (strcmp(file_type_map[format_struct_index].format_cstring, driver) == 0) {
            format = file_type_map[format_struct_index].format_enum;
            break;
        }
    }

    if (format == 0) {
        return false;
    }

    sfinfo.format = format | sfinfo.format;

    if (sf_format_check(&sfinfo)) {
        bits = 32;
        return true;
    }

    int temp_format = 0;
    const int format_count = 2;

    for (int format_attempt = 0; format_attempt < format_count; format_attempt++) {
        if (format == SF_FORMAT_XI) {
            sfinfo.channels = 1;
            if (format_attempt == 0) {
                temp_format = format | SF_FORMAT_DPCM_16;
                bits = 16;
            } else {
                temp_format = format | SF_FORMAT_DPCM_8;
                bits = 8;
            }
        } else {
            if (format_attempt == 0) {
                temp_format = format | SF_FORMAT_PCM_24;
                bits = 24;
            } else {
                temp_format = format | SF_FORMAT_PCM_16;
                bits = 16;
            }
        }

        sfinfo.format = temp_format;

        if (sf_format_check(&sfinfo)) {
            return true;
        }
    }

    return false;
}

} // namespace

SndfileSink::SndfileSink(core::IArena& arena, const Config& config)
    : sndfile_output_(NULL)
    , buffer_(arena)
    , buffer_size_(0)
    , is_file_(true)
    , valid_(false) {
    BackendMap::instance();

    if (config.sample_spec.num_channels() == 0) {
        roc_log(LogError, "sndfile sink: # of channels is zero");
        return;
    }

    if (config.latency != 0) {
        roc_log(LogError,
                "sndfile sink: setting io latency not supported by sndfile backend");
        return;
    }

    frame_length_ = config.frame_length;
    sample_spec_ = config.sample_spec;

    if (frame_length_ == 0) {
        roc_log(LogError, "sndfile sink: frame length is zero");
        return;
    }

    memset(&sf_info_out_, 0, sizeof(sf_info_out_));

    sf_info_out_.format = SF_FORMAT_PCM_32;
    sf_info_out_.channels = (int)config.sample_spec.num_channels();
    sf_info_out_.samplerate = (int)config.sample_spec.sample_rate();

    valid_ = true;
}

SndfileSink::~SndfileSink() {
    close_();
}

bool SndfileSink::is_valid() const {
    return valid_;
}

bool SndfileSink::open(const char* driver, const char* path) {
    roc_panic_if(!valid_);

    roc_log(LogDebug, "sndfile sink: opening: driver=%s path=%s", driver, path);

    if (buffer_.size() != 0 || sndfile_output_) {
        roc_panic("sndfile sink: can't call open() more than once");
    }

    if (!open_(driver, path)) {
        return false;
    }

    if (!setup_buffer_()) {
        return false;
    }

    return true;
}

DeviceType SndfileSink::type() const {
    return DeviceType_Sink;
}

DeviceState SndfileSink::state() const {
    return DeviceState_Active;
}

void SndfileSink::pause() {
    // no-op
}

bool SndfileSink::resume() {
    return true;
}

bool SndfileSink::restart() {
    return true;
}

audio::SampleSpec SndfileSink::sample_spec() const {
    roc_panic_if(!valid_);

    if (!sndfile_output_) {
        roc_panic("sndfile sink: sample_rate(): non-open output file or device");
    }

    if (sf_info_out_.channels == 1) {
        return audio::SampleSpec(size_t(sf_info_out_.samplerate),
                                 audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                 audio::ChanMask_Surround_Mono);
    }

    if (sf_info_out_.channels == 2) {
        return audio::SampleSpec(size_t(sf_info_out_.samplerate),
                                 audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                 audio::ChanMask_Surround_Stereo);
    }

    roc_panic("sndfile sink: unsupported channel count");
}

core::nanoseconds_t SndfileSink::latency() const {
    roc_panic_if(!valid_);

    if (!sndfile_output_) {
        roc_panic("sndfile sink: latency(): non-open output file or device");
    }

    return 0;
}

bool SndfileSink::has_latency() const {
    roc_panic_if(!valid_);

    if (!sndfile_output_) {
        roc_panic("sndfile sink: has_latency(): non-open output file or device");
    }

    return false;
}

bool SndfileSink::has_clock() const {
    roc_panic_if(!valid_);

    if (!sndfile_output_) {
        roc_panic("sndfile sink: has_clock(): non-open output file or device");
    }

    return !is_file_;
}

void SndfileSink::write(audio::Frame& frame) {
    roc_panic_if(!valid_);
    const audio::sample_t* frame_data = frame.samples();
    size_t frame_size = frame.num_samples();
    audio::sample_t* buffer_data = buffer_.data();
    size_t buffer_pos = 0;

    while (frame_size > 0) {
        for (; buffer_pos < buffer_size_ && frame_size > 0; buffer_pos++) {
            buffer_data[buffer_pos] = *frame_data;
            frame_data++;
            frame_size--;
        }

        if (buffer_pos > 0) {
            if (sf_write_float(sndfile_output_, buffer_data, (sf_count_t)buffer_size_)
                    == 0
                && frame_size > 0) {
                roc_panic(
                    "sndfile sink: Unable to write entire frame to file. Reached eof");
            }
        }
        buffer_pos = 0;
    }
}

bool SndfileSink::setup_buffer_() {
    buffer_size_ = sample_spec_.ns_2_samples_overall(frame_length_);
    if (buffer_size_ == 0) {
        roc_log(LogError, "sndfile sink: buffer size is zero");
        return false;
    }
    if (!buffer_.resize(buffer_size_)) {
        roc_log(LogError, "sndfile sink: can't allocate sample buffer");
        return false;
    }

    return true;
}

bool SndfileSink::open_(const char* driver, const char* path) {
    unsigned long in_rate = (unsigned long)sf_info_out_.samplerate;

    if (sf_info_out_.samplerate == 0) {
        sf_info_out_.samplerate = 48000;
    }

    unsigned long out_rate = (unsigned long)sf_info_out_.samplerate;

    if (!driver) {
        if (!detect_file_extension(&driver, path)) {
            roc_log(LogDebug, "sndfile sink: Driver extension could not be detected");
            return false;
        }
    }

    int bits = 0;
    if (!map_to_sndfile(sf_info_out_, driver, bits)) {
        roc_log(LogDebug,
                "sndfile sink: Cannot find valid subtype format for major format type");
        return false;
    }

    sndfile_output_ = sf_open(path, SFM_WRITE, &sf_info_out_);
    if (!sndfile_output_) {
        roc_log(LogDebug, "sndfile sink: can't open: driver=%s path=%s", driver, path);
        return false;
    }

    sf_command(sndfile_output_, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE);

    if (in_rate != 0 && in_rate != out_rate) {
        roc_log(LogError,
                "sndfile sink:"
                " can't open output file or device with the required sample rate:"
                " required_by_output=%lu requested_by_user=%lu",
                out_rate, in_rate);
        return false;
    }
    sample_spec_.set_sample_rate((unsigned long)sf_info_out_.samplerate);

    roc_log(LogInfo,
            "sndfile sink:"
            " opened: bits=%lu out_rate=%lu in_rate=%lu ch=%lu is_file=%d",
            (unsigned long)bits, out_rate, in_rate, (unsigned long)sf_info_out_.channels,
            (int)is_file_);

    sf_seek(sndfile_output_, 0, SEEK_SET);

    return true;
}

void SndfileSink::close_() {
    if (!sndfile_output_) {
        return;
    }

    roc_log(LogDebug, "sndfile sink: closing output");

    int err = sf_close(sndfile_output_);
    if (err != 0) {
        roc_panic("sndfile sink: can't close output: %s", sf_error_number(err));
    }

    sndfile_output_ = NULL;
}

} // namespace sndio
} // namespace roc
