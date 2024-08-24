/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_helpers.h"
#include "roc_sndio/sndfile_tables.h"

namespace roc {
namespace sndio {

namespace {

int pcm_2_sf(audio::PcmSubformat fmt) {
    switch (fmt) {
    case audio::PcmSubformat_UInt8:
    case audio::PcmSubformat_UInt8_Le:
    case audio::PcmSubformat_UInt8_Be:
        return SF_FORMAT_PCM_U8 | SF_ENDIAN_FILE;

    case audio::PcmSubformat_SInt8:
    case audio::PcmSubformat_SInt8_Le:
    case audio::PcmSubformat_SInt8_Be:
        return SF_FORMAT_PCM_S8 | SF_ENDIAN_FILE;

    case audio::PcmSubformat_SInt16:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_FILE;
    case audio::PcmSubformat_SInt16_Le:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
    case audio::PcmSubformat_SInt16_Be:
        return SF_FORMAT_PCM_16 | SF_ENDIAN_BIG;

    case audio::PcmSubformat_SInt24:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_FILE;
    case audio::PcmSubformat_SInt24_Le:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_LITTLE;
    case audio::PcmSubformat_SInt24_Be:
        return SF_FORMAT_PCM_24 | SF_ENDIAN_BIG;

    case audio::PcmSubformat_SInt32:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_FILE;
    case audio::PcmSubformat_SInt32_Le:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_LITTLE;
    case audio::PcmSubformat_SInt32_Be:
        return SF_FORMAT_PCM_32 | SF_ENDIAN_BIG;

    case audio::PcmSubformat_Float32:
        return SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
    case audio::PcmSubformat_Float32_Le:
        return SF_FORMAT_FLOAT | SF_ENDIAN_LITTLE;
    case audio::PcmSubformat_Float32_Be:
        return SF_FORMAT_FLOAT | SF_ENDIAN_BIG;

    case audio::PcmSubformat_Float64:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_FILE;
    case audio::PcmSubformat_Float64_Le:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_LITTLE;
    case audio::PcmSubformat_Float64_Be:
        return SF_FORMAT_DOUBLE | SF_ENDIAN_BIG;

    default:
        break;
    }

    return 0;
}

} // namespace

status::StatusCode sndfile_select_major_format(SF_INFO& file_info,
                                               audio::SampleSpec& sample_spec,
                                               const char* path) {
    roc_panic_if(!path);

    const char* file_extension = NULL;
    const char* dot = strrchr(path, '.');
    if (dot && dot != path) {
        file_extension = dot;
    }

    // First try to select format by iterating through sndfile_driver_remap.
    if (sample_spec.has_format()) {
        // If format is specified, match by format name.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_format_remap); idx++) {
            if (strcmp(sample_spec.format_name(), sndfile_format_remap[idx].name) == 0) {
                file_info.format = sndfile_format_remap[idx].format_mask;
                return status::StatusOK;
            }
        }
    } else if (file_extension != NULL) {
        // If format is omitted, match by file extension.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_format_remap); idx++) {
            if (sndfile_format_remap[idx].file_extension != NULL) {
                if (strcmp(file_extension, sndfile_format_remap[idx].file_extension)
                    == 0) {
                    file_info.format = sndfile_format_remap[idx].format_mask;
                    if (!sample_spec.set_custom_format(sndfile_format_remap[idx].name)) {
                        continue;
                    }
                    return status::StatusOK;
                }
            }
        }
    }

    // Then try to select format by iterating through all sndfile major formats.
    int major_format_count = 0;
    if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_format_count,
                                sizeof(int))) {
        roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                  sf_error_number(errnum));
    }

    for (int idx = 0; idx < major_format_count; idx++) {
        SF_FORMAT_INFO format_info;
        memset(&format_info, 0, sizeof(format_info));
        format_info.format = idx;
        if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info,
                                    sizeof(format_info))) {
            roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                      sf_error_number(errnum));
        }

        if (sample_spec.has_format()) {
            // If format is specified, match by format name.
            // Note that format name = file extension.
            if (strcmp(format_info.extension, sample_spec.format_name()) == 0) {
                file_info.format = format_info.format;
                return status::StatusOK;
            }
        } else if (file_extension != NULL) {
            // If format is omitted, match by file extension.
            if (strcmp(format_info.extension, file_extension) == 0) {
                file_info.format = format_info.format;
                if (!sample_spec.set_custom_format(format_info.name)) {
                    continue;
                }
                return status::StatusOK;
            }
        }
    }

    if (sample_spec.has_format()) {
        roc_log(
            LogDebug,
            "sndfile backend: requested format '%s' not supported by backend: path=%s",
            sample_spec.format_name(), path);
    } else {
        roc_log(LogDebug,
                "sndfile backend: can't detect file format from extension: path=%s",
                path);
    }
    // Not a wav file, go to next backend.
    return status::StatusNoFormat;
}

status::StatusCode sndfile_select_sub_format(SF_INFO& file_info,
                                             audio::SampleSpec& sample_spec,
                                             const char* path) {
    roc_panic_if(!path);

    const int format_mask = file_info.format;

    // If sub-format is specified, use it.
    if (sample_spec.has_subformat()) {
        int subformat_mask = 0;

        if (sample_spec.pcm_subformat() != audio::PcmSubformat_Invalid) {
            // PCM sub-formats are mapped by enum.
            subformat_mask = pcm_2_sf(sample_spec.pcm_subformat());
        } else {
            // Other sub-formats are mapped by string name.
            for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_subformat_map); idx++) {
                if (strcmp(sample_spec.subformat_name(), sndfile_subformat_map[idx].name)
                    == 0) {
                    subformat_mask = sndfile_subformat_map[idx].format_mask;
                    break;
                }
            }
        }

        if (subformat_mask != 0) {
            file_info.format = format_mask | subformat_mask;

            if (sf_format_check(&file_info)) {
                return status::StatusOK;
            }
        }

        roc_log(LogError,
                "sndfile backend: invalid io encoding:"
                " <subformat> '%s' not allowed when <format> is '%s'",
                sample_spec.subformat_name(), sample_spec.format_name());
        return status::StatusBadConfig;
    }

    // For some formats, sub-format must be always specified explicitly.
    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_explicit_formats); idx++) {
        if (file_info.format == sndfile_explicit_formats[idx]) {
            roc_log(LogError,
                    "sndfile backend: invalid io encoding:"
                    " <subformat> is required when <format> is '%s'",
                    sample_spec.format_name());
            return status::StatusBadConfig;
        }
    }

    // If sub-format is omitted, first try if we can work without sub-format.
    file_info.format = format_mask;

    if (sf_format_check(&file_info)) {
        return status::StatusOK;
    }

    // We can't work without sub-format, choose one of the default sub-formats.
    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_default_subformats); idx++) {
        const int subformat_mask = sndfile_default_subformats[idx];

        file_info.format = format_mask | subformat_mask;

        if (sf_format_check(&file_info)) {
            return status::StatusOK;
        }
    }

    roc_log(LogError,
            "sndfile backend: invalid io encoding:"
            " <subformat> is required when <format> is '%s'",
            sample_spec.format_name());
    return status::StatusBadConfig;
}

status::StatusCode sndfile_check_input_spec(const SF_INFO& file_info,
                                            const audio::SampleSpec& sample_spec,
                                            const char* path) {
    roc_panic_if(!path);

    bool is_explicit = false;

    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_explicit_formats); idx++) {
        if (file_info.format == sndfile_explicit_formats[idx]) {
            is_explicit = true;
            break;
        }
    }

    if (is_explicit) {
        if (!sample_spec.has_subformat() || !sample_spec.has_sample_rate()
            || !sample_spec.has_channel_set()) {
            roc_log(LogError,
                    "sndfile backend: invalid io encoding: <subformat>, <rate> and"
                    " <channels> required for input file when <format> is '%s'",
                    sample_spec.format_name());
            return status::StatusBadConfig;
        }
    } else {
        if (sample_spec.has_subformat() || sample_spec.has_sample_rate()
            || sample_spec.has_channel_set()) {
            roc_log(LogError,
                    "sndfile backend: invalid io encoding: <subformat>, <rate> and"
                    " <channels> not allowed for input file when <format> is '%s'",
                    sample_spec.format_name());
            return status::StatusBadConfig;
        }
    }

    return status::StatusOK;
}

status::StatusCode sndfile_detect_format(const SF_INFO& file_info,
                                         audio::SampleSpec& sample_spec) {
    if (!sample_spec.has_format()) {
        // First check sndfile_driver_remap.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_format_remap); idx++) {
            if ((file_info.format & sndfile_format_remap[idx].format_mask)
                == sndfile_format_remap[idx].format_mask) {
                if (!sample_spec.set_custom_format(sndfile_format_remap[idx].name)) {
                    continue;
                }
                break;
            }
        }

        // Then check rest major formats.
        int major_format_count = 0;
        if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_format_count,
                                    sizeof(int))) {
            roc_panic(
                "sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                sf_error_number(errnum));
        }

        for (int idx = 0; idx < major_format_count; idx++) {
            SF_FORMAT_INFO format_info;
            memset(&format_info, 0, sizeof(format_info));
            format_info.format = idx;
            if (int errnum = sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info,
                                        sizeof(format_info))) {
                roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                          sf_error_number(errnum));
            }

            if ((file_info.format & format_info.format) == format_info.format) {
                if (!sample_spec.set_custom_format(format_info.extension)) {
                    continue;
                }
                break;
            }
        }

        if (!sample_spec.has_format()) {
            roc_log(LogError, "sndfile backend: can't detect file format");
            return status::StatusErrFile;
        }
    }

    if (!sample_spec.has_subformat()) {
        // First check pcm sub-formats.
        for (int subfmt = audio::PcmSubformat_Invalid; subfmt < audio::PcmSubformat_Max;
             subfmt++) {
            const int subfmt_mask =
                pcm_2_sf((audio::PcmSubformat)subfmt) & SF_FORMAT_SUBMASK;

            if ((file_info.format & subfmt_mask) == subfmt_mask) {
                sample_spec.set_pcm_subformat((audio::PcmSubformat)subfmt);
                break;
            }
        }

        // Then check rest sub-formats.
        for (size_t idx = 0; idx < ROC_ARRAY_SIZE(sndfile_subformat_map); idx++) {
            if ((file_info.format & sndfile_subformat_map[idx].format_mask)
                == sndfile_subformat_map[idx].format_mask) {
                if (!sample_spec.set_custom_subformat(sndfile_subformat_map[idx].name)) {
                    continue;
                }
                break;
            }
        }
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
