/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_extension_table.h"
#include "roc_core/macro_helpers.h"

#include <sndfile.h>

namespace roc {
namespace sndio {

FileMap file_type_map[ROC_ARRAY_SIZE(file_type_map)] = {
    { SF_FORMAT_MAT4, "mat4", NULL },   //
    { SF_FORMAT_MAT5, "mat5", NULL },   //
    { SF_FORMAT_WAV, "wav", "wav" },    //
    { SF_FORMAT_NIST, "nist", NULL },   //
    { SF_FORMAT_WAVEX, "wavex", NULL }, //
};

} // namespace sndio
} // namespace roc
