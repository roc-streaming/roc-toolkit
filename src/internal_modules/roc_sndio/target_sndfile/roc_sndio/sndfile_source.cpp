/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"

namespace roc {
namespace sndio {
namespace {
struct BitMap {
    int sub_type;
    int bit_depth;
} bit_map[] = {
    { SF_FORMAT_PCM_S8, 8 },    { SF_FORMAT_PCM_16, 16 },  { SF_FORMAT_PCM_24, 24 },
    { SF_FORMAT_PCM_S8, 32 },   { SF_FORMAT_PCM_U8, 8 },   { SF_FORMAT_FLOAT, 32 },
    { SF_FORMAT_DOUBLE, 64 },   { SF_FORMAT_ULAW, 8 },     { SF_FORMAT_ALAW, 8 },
    { SF_FORMAT_IMA_ADPCM, 4 }, { SF_FORMAT_MS_ADPCM, 4 }, { SF_FORMAT_GSM610, 13 },
    { SF_FORMAT_VOX_ADPCM, 4 }, { SF_FORMAT_G721_32, 4 },  { SF_FORMAT_G723_24, 4 },
    { SF_FORMAT_G723_40, 4 },   { SF_FORMAT_DWVW_12, 12 }, { SF_FORMAT_DWVW_16, 16 },
    { SF_FORMAT_DWVW_24, 24 },  { SF_FORMAT_DPCM_8, 8 },   { SF_FORMAT_DPCM_16, 16 },
    { SF_FORMAT_VORBIS, 32 },   { SF_FORMAT_DPCM_8, 8 },   { SF_FORMAT_DPCM_16, 16 },
};

bool find_subtype(int& format_subtype, SNDFILE* sndfile_input_, SF_INFO sf_info_in_) {
    SF_FORMAT_INFO format_info_major, format_info_subtype;
    int major_count, subtype_count;

    sf_command(sndfile_input_, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int));
    sf_command(sndfile_input_, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof(int));

    for (int maj_index = 0; maj_index < major_count; maj_index++) {
        format_info_major.format = maj_index;
        sf_command(sndfile_input_, SFC_GET_FORMAT_MAJOR, &format_info_major,
                   sizeof(format_info_major));

        for (int subtype_index = 0; subtype_index < subtype_count; subtype_index++) {
            format_info_subtype.format = subtype_index;
            sf_command(sndfile_input_, SFC_GET_FORMAT_SUBTYPE, &format_info_subtype,
                       sizeof(format_info_subtype));

            if ((format_info_major.format | format_info_subtype.format)
                == sf_info_in_.format) {
                format_subtype = format_info_subtype.format;
                return true;
            }
        }
    }
    return false;
}

bool map_to_bit_depth(int& in_bits, int format_subtype) {
    for (size_t bit_map_index = 0; bit_map_index < ROC_ARRAY_SIZE(bit_map);
         bit_map_index++) {
        if (bit_map[bit_map_index].sub_type == format_subtype) {
            in_bits = bit_map[bit_map_index].bit_depth;
            return true;
        }
    }
    return false;
}
} // namespace

SndfileSource::SndfileSource(core::IArena& arena, const Config& config)
    : driver_name_(arena)
    , input_name_(arena)
    , buffer_(arena)
    , buffer_size_(0)
    , sndfile_input_(NULL)
    , is_file_(true)
    , eof_(false)
    , paused_(false)
    , valid_(false) {
    BackendMap::instance();

    if (config.sample_spec.num_channels() == 0) {
        roc_log(LogError, "sndfile source: # of channels is zero");
        return;
    }

    if (config.latency != 0) {
        roc_log(LogError,
                "sndfile source: setting io latency not supported by sndfile backend");
        return;
    }

    frame_length_ = config.frame_length;
    sample_spec_ = config.sample_spec;

    if (frame_length_ == 0) {
        roc_log(LogError, "sndfile source: frame length is zero");
        return;
    }

    memset(&sf_info_in_, 0, sizeof(sf_info_in_));
    sample_rate_ = (int)config.sample_spec.sample_rate();
    precision_ = 32;

    valid_ = true;
}

SndfileSource::~SndfileSource() {
    close_();
}

bool SndfileSource::is_valid() const {
    return valid_;
}

bool SndfileSource::open(const char* driver, const char* path) {
    roc_panic_if(!valid_);

    roc_log(LogInfo, "sndfile source: opening: driver=%s path=%s", driver, path);

    if (buffer_.size() != 0 || sndfile_input_) {
        roc_panic("sndfile source: can't call open() more than once");
    }

    if (!setup_names_(driver, path)) {
        return false;
    }

    if (!open_()) {
        return false;
    }

    if (!setup_buffer_()) {
        return false;
    }

    return true;
}

DeviceType SndfileSource::type() const {
    return DeviceType_Source;
}

DeviceState SndfileSource::state() const {
    roc_panic_if(!valid_);

    if (paused_) {
        return DeviceState_Paused;
    } else {
        return DeviceState_Active;
    }
}

void SndfileSource::pause() {
    roc_panic_if(!valid_);

    if (paused_) {
        return;
    }

    if (!sndfile_input_) {
        roc_panic("sndfile source: pause: non-open input file or device");
    }

    roc_log(LogDebug, "sndfile source: pausing: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (!is_file_) {
        close_();
    }

    paused_ = true;
}

bool SndfileSource::resume() {
    roc_panic_if(!valid_);

    if (!paused_) {
        return true;
    }

    roc_log(LogDebug, "sndfile source: resuming: driver=%s input=%s",
            driver_name_.c_str(), input_name_.c_str());

    if (!sndfile_input_) {
        if (!open_()) {
            roc_log(LogError,
                    "sndfile source: open failed when resuming: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    }

    paused_ = false;
    return true;
}

bool SndfileSource::restart() {
    roc_panic_if(!valid_);

    roc_log(LogDebug, "sndfile source: restarting: driver=%s input=%s",
            driver_name_.c_str(), input_name_.c_str());

    if (is_file_ && !eof_) {
        if (!seek_(0)) {
            roc_log(LogError,
                    "sndfile source: seek failed when restarting: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    } else {
        if (sndfile_input_) {
            close_();
        }

        if (!open_()) {
            roc_log(LogError,
                    "sndfile source: open failed when restarting: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    }

    paused_ = false;
    eof_ = false;

    return true;
}

audio::SampleSpec SndfileSource::sample_spec() const {
    roc_panic_if(!valid_);

    if (!sndfile_input_) {
        roc_panic("sndfile source: sample_rate(): non-open output file or device");
    }

    if (sf_info_in_.channels == 1) {
        return audio::SampleSpec(size_t(sf_info_in_.samplerate),
                                 audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                 audio::ChanMask_Surround_Mono);
    }

    if (sf_info_in_.channels == 2) {
        return audio::SampleSpec(size_t(sf_info_in_.samplerate),
                                 audio::ChanLayout_Surround, audio::ChanOrder_Smpte,
                                 audio::ChanMask_Surround_Stereo);
    }

    roc_panic("sndfile source: unsupported channel count");
}

core::nanoseconds_t SndfileSource::latency() const {
    roc_panic_if(!valid_);

    if (!sndfile_input_) {
        roc_panic("sndfile source: latency(): non-open output file or device");
    }

    return 0;
}

bool SndfileSource::has_latency() const {
    roc_panic_if(!valid_);

    if (!sndfile_input_) {
        roc_panic("sndfile source: has_latency(): non-open input file or device");
    }

    return false;
}

bool SndfileSource::has_clock() const {
    roc_panic_if(!valid_);

    if (!sndfile_input_) {
        roc_panic("sndfile source: has_clock(): non-open input file or device");
    }

    return !is_file_;
}

void SndfileSource::reclock(core::nanoseconds_t) {
    // no-op
}

bool SndfileSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (paused_ || eof_) {
        return false;
    }

    if (!sndfile_input_) {
        roc_panic("sndfile source: read: non-open input file or device");
    }

    audio::sample_t* frame_data = frame.samples();
    size_t frame_left = frame.num_samples();

    audio::sample_t* buffer_data = buffer_.data();

    while (frame_left != 0) {
        size_t n_samples =
            (size_t)sf_read_float(sndfile_input_, buffer_data, (sf_count_t)buffer_size_);

        if (n_samples == 0) {
            roc_log(LogDebug, "sndfile source: got eof from sndfile");
            eof_ = true;
            break;
        }

        for (size_t n = 0; n < n_samples; n++) {
            frame_data[n] = buffer_data[n];
        }

        frame_data += n_samples;
        frame_left -= n_samples;
    }

    if (frame_left == frame.num_samples()) {
        return false;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return true;
}

bool SndfileSource::seek_(size_t offset) {
    roc_panic_if(!valid_);

    if (!sndfile_input_) {
        roc_panic("sndfile source: seek: non-open input file or device");
    }

    if (!is_file_) {
        roc_panic("sndfile source: seek: not a file");
    }

    roc_log(LogDebug, "sndfile source: resetting position to %lu", (unsigned long)offset);

    sf_count_t err = sf_seek(sndfile_input_, (sf_count_t)offset, SEEK_SET);
    if (err == -1) {
        roc_log(LogError,
                "sndfile source: can't reset position to %lu: an attempt was made to "
                "seek beyond the start or end of the file",
                (unsigned long)offset);
        return false;
    }

    return true;
}

bool SndfileSource::setup_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_name_.assign(driver)) {
            roc_log(LogError, "sndfile source: can't allocate string");
            return false;
        }
    }

    if (path) {
        if (!input_name_.assign(path)) {
            roc_log(LogError, "sndfile source: can't allocate string");
            return false;
        }
    }

    return true;
}

bool SndfileSource::setup_buffer_() {
    buffer_size_ = sample_spec_.ns_2_samples_overall(frame_length_);
    if (buffer_size_ == 0) {
        roc_log(LogError, "sndfile source: buffer size is zero");
        return false;
    }
    if (!buffer_.resize(buffer_size_)) {
        roc_log(LogError, "sndfile source: can't allocate sample buffer");
        return false;
    }

    return true;
}

bool SndfileSource::open_() {
    if (sndfile_input_) {
        roc_panic("sndfile source: already opened");
    }

    sf_info_in_.format = 0;

    sndfile_input_ = sf_open(input_name_.is_empty() ? NULL : input_name_.c_str(),
                             SFM_READ, &sf_info_in_);
    if (!sndfile_input_) {
        roc_log(LogInfo, "sndfile source: can't open: driver=%s input=%s",
                driver_name_.c_str(), input_name_.c_str());
        return false;
    }

    if (sf_info_in_.channels != (int)sample_spec_.num_channels()) {
        roc_log(LogError,
                "sndfile source: can't open: unsupported # of channels: "
                "expected=%lu actual=%lu",
                (unsigned long)sample_spec_.num_channels(),
                (unsigned long)sf_info_in_.channels);
        return false;
    }

    if (sample_rate_ != 0) {
        sf_info_in_.samplerate = sample_rate_;
    }

    sample_spec_.set_sample_rate((unsigned long)sf_info_in_.samplerate);

    int in_bits = 0;
    int format_subtype = 0;

    if (!find_subtype(format_subtype, sndfile_input_, sf_info_in_)) {
        roc_log(LogDebug, "sndfile source: FORMAT_SUBTYPE could not be detected.");
    }

    if (!map_to_bit_depth(in_bits, format_subtype)) {
        roc_log(LogDebug, "sndfile source: in_bits could not be detected.");
    }

    roc_log(LogInfo,
            "sndfile source:"
            " in_bits=%lu out_bits=%lu in_rate=%lu out_rate=%lu"
            " in_ch=%lu out_ch=%lu is_file=%d",
            (unsigned long)in_bits, (unsigned long)precision_,
            (unsigned long)sf_info_in_.samplerate, (unsigned long)sample_rate_,
            (unsigned long)sf_info_in_.channels, (unsigned long)0, (int)is_file_);

    return true;
}

void SndfileSource::close_() {
    if (!sndfile_input_) {
        return;
    }

    roc_log(LogInfo, "sndfile source: closing input");

    int err = sf_close(sndfile_input_);
    if (err != 0) {
        roc_panic("sndfile source: can't close input: %s", sf_error_number(err));
    }

    sndfile_input_ = NULL;
}

} // namespace sndio
} // namespace roc
