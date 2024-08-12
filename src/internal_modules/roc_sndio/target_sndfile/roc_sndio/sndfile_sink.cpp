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
#include "roc_sndio/sndfile_tables.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

int pcm_2_sf(audio::PcmFormat fmt) {
    switch (fmt) {
    case audio::PcmFormat_UInt8:
    case audio::PcmFormat_UInt8_Le:
    case audio::PcmFormat_UInt8_Be:
        return SF_FORMAT_PCM_U8 | SF_ENDIAN_FILE;

    case audio::PcmFormat_SInt8:
    case audio::PcmFormat_SInt8_Le:
    case audio::PcmFormat_SInt8_Be:
        return SF_FORMAT_PCM_S8 | SF_ENDIAN_FILE;

    case audio::PcmFormat_SInt16:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_FILE;
    case audio::PcmFormat_SInt16_Le:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
    case audio::PcmFormat_SInt16_Be:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_BIG;

    case audio::PcmFormat_SInt24:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_FILE;
    case audio::PcmFormat_SInt24_Le:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_LITTLE;
    case audio::PcmFormat_SInt24_Be:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_BIG;

    case audio::PcmFormat_SInt32:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_FILE;
    case audio::PcmFormat_SInt32_Le:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_LITTLE;
    case audio::PcmFormat_SInt32_Be:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_BIG;

    case audio::PcmFormat_Float32:
        return SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
    case audio::PcmFormat_Float32_Le:
        return SF_FORMAT_FLOAT | SF_ENDIAN_LITTLE;
    case audio::PcmFormat_Float32_Be:
        return SF_FORMAT_FLOAT | SF_ENDIAN_BIG;

    case audio::PcmFormat_Float64:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_FILE;
    case audio::PcmFormat_Float64_Le:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_LITTLE;
    case audio::PcmFormat_Float64_Be:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_BIG;

    default:
        break;
    }

    return 0;
}

bool select_major_format(SF_INFO& file_info, const char** driver, const char* path) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    const char* file_extension = NULL;

    const char* dot = strrchr(path, '.');
    if (dot && dot != path) {
        file_extension = dot;
    }

    int format_mask = 0;

    // First try to select format by iterating through sndfile_driver_remap.
    if (*driver != NULL) {
        // If driver is non-NULL, match by driver name.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_driver_remap); idx++) {
            if (strcmp(*driver, sndfile_driver_remap[idx].driver_name) == 0) {
                format_mask = sndfile_driver_remap[idx].format_mask;
                break;
            }
        }
    } else if (file_extension != NULL) {
        // If driver is NULL, match by file extension.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_driver_remap); idx++) {
            if (sndfile_driver_remap[idx].file_extension != NULL) {
                if (strcmp(file_extension, sndfile_driver_remap[idx].file_extension)
                    == 0) {
                    format_mask = sndfile_driver_remap[idx].format_mask;
                    *driver = file_extension;
                    break;
                }
            }
        }
    }

    // Then try to select format by iterating through all sndfile formats.
    if (format_mask == 0) {
        int major_count = 0;
        if (int errnum =
                sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int))) {
            roc_panic(
                "sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                sf_error_number(errnum));
        }

        for (int format_index = 0; format_index < major_count; format_index++) {
            SF_FORMAT_INFO info;
            memset(&info, 0, sizeof(info));
            info.format = format_index;
            if (int errnum =
                    sf_command(NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof(info))) {
                roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                          sf_error_number(errnum));
            }

            if (*driver != NULL) {
                // If driver is non-NULL, match by driver name.
                if (strcmp(info.extension, *driver) == 0) {
                    format_mask = info.format;
                    break;
                }
            } else if (file_extension != NULL) {
                // If driver is NULL, match by file extension.
                if (strcmp(info.extension, file_extension) == 0) {
                    format_mask = info.format;
                    *driver = file_extension;
                    break;
                }
            }
        }
    }

    if (format_mask == 0) {
        roc_log(LogDebug,
                "sndfile sink: failed to detect major format: driver=%s path=%s", *driver,
                path);
        return false;
    }

    file_info.format = format_mask;

    return true;
}

bool select_sub_format(SF_INFO& file_info,
                       const char* driver,
                       const char* path,
                       int requested_subformat) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    const int format_mask = file_info.format;

    // User explicitly requested specific PCM format, map it to sub-format.
    if (requested_subformat != 0) {
        file_info.format = format_mask | requested_subformat;

        if (sf_format_check(&file_info)) {
            return true;
        }

        roc_log(LogError,
                "sndfile sink: requested format not supported by driver:"
                " driver=%s path=%s",
                driver, path);
        return false;
    }

    // User did not request specific PCM format.
    // First check if we can work without sub-format.
    file_info.format = format_mask;

    if (sf_format_check(&file_info)) {
        return true;
    }

    // We can't work without sub-format and should choose one.
    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_default_subformats); idx++) {
        const int sub_format = sndfile_default_subformats[idx];

        file_info.format = format_mask | sub_format;

        if (sf_format_check(&file_info)) {
            return true;
        }
    }

    roc_log(LogDebug, "sndfile sink: failed to detect sub-format: driver=%s path=%s",
            driver, path);

    return false;
}

} // namespace

SndfileSink::SndfileSink(audio::FrameFactory& frame_factory,
                         core::IArena& arena,
                         const IoConfig& io_config)
    : file_(NULL)
    , requested_subformat_(0)
    , init_status_(status::NoStatus) {
    if (io_config.latency != 0) {
        roc_log(LogError, "sndfile sink: setting io latency not supported by backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.sample_format() != audio::SampleFormat_Invalid
        && io_config.sample_spec.sample_format() != audio::SampleFormat_Pcm) {
        roc_log(LogError, "sndfile sink: requested format not supported by backend: %s",
                audio::sample_spec_to_str(sample_spec_).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.is_pcm()) {
        // Remember which format to use for file, if requested explicitly.
        requested_subformat_ = pcm_2_sf(io_config.sample_spec.pcm_format());
        if (requested_subformat_ == 0) {
            roc_log(LogError,
                    "sndfile sink: requested format not supported by backend: %s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    sample_spec_ = io_config.sample_spec;

    // Always request raw samples from pipeline.
    // If the user requested different format, we've remembered it above and will
    // tell libsndfile to perform conversion to that format when writing file.
    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);

    sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                              44100);

    memset(&file_info_, 0, sizeof(file_info_));

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

status::StatusCode SndfileSink::open(const char* driver, const char* path) {
    roc_log(LogDebug, "sndfile sink: opening: driver=%s path=%s", driver, path);

    if (file_) {
        roc_panic("sndfile sink: can't call open() more than once");
    }

    if (!path) {
        roc_panic("sndfile sink: path is null");
    }

    return open_(driver, path);
}

status::StatusCode SndfileSink::close() {
    return close_();
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

    return sample_spec_;
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

status::StatusCode SndfileSink::open_(const char* driver, const char* path) {
    file_info_.channels = (int)sample_spec_.num_channels();
    file_info_.samplerate = (int)sample_spec_.sample_rate();

    if (!select_major_format(file_info_, &driver, path)) {
        roc_log(LogDebug, "sndfile sink: can't detect file format: driver=%s path=%s",
                driver, path);
        return status::StatusErrFile;
    }

    if (!select_sub_format(file_info_, driver, path, requested_subformat_)) {
        roc_log(LogDebug, "sndfile sink: can't detect file format: driver=%s path=%s",
                driver, path);
        return status::StatusErrFile;
    }

    file_ = sf_open(path, SFM_WRITE, &file_info_);
    if (!file_) {
        roc_log(LogDebug, "sndfile sink: %s, can't open file: driver=%s path=%s",
                sf_strerror(file_), driver, path);
        return status::StatusErrFile;
    }

    if (!sf_command(file_, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE)) {
        roc_log(LogDebug, "sndfile sink: %s, can't configure driver: driver=%s path=%s",
                sf_strerror(file_), driver, path);
        return status::StatusErrFile;
    }

    roc_log(LogInfo, "sndfile sink: opened output file: %s",
            audio::sample_spec_to_str(sample_spec_).c_str());

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
