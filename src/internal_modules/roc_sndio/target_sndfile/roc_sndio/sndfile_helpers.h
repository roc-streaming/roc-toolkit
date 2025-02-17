/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_helpers.h
//! @brief Sndfile helpers.

#ifndef ROC_SNDIO_SNDFILE_HELPERS_H_
#define ROC_SNDIO_SNDFILE_HELPERS_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/attributes.h"
#include "roc_status/status_code.h"

#include <sndfile.h>

namespace roc {
namespace sndio {

//! Choose sndfile major format from sample spec and path.
ROC_ATTR_NODISCARD status::StatusCode sndfile_select_major_format(
    SF_INFO& file_info, audio::SampleSpec& sample_spec, const char* path);

//! Choose sndfile sub-format from sample spec and path.
ROC_ATTR_NODISCARD status::StatusCode sndfile_select_sub_format(
    SF_INFO& file_info, audio::SampleSpec& sample_spec, const char* path);

//! Check that requested specification is valid for given input file.
ROC_ATTR_NODISCARD status::StatusCode sndfile_check_input_spec(
    const SF_INFO& file_info, const audio::SampleSpec& sample_spec, const char* path);

//! Detect format and sub-format of opened file and fill sample spec.
ROC_ATTR_NODISCARD status::StatusCode
sndfile_detect_format(const SF_INFO& file_info, audio::SampleSpec& sample_spec);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_HELPERS_H_
