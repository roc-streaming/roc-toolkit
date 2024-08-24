/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_tables.h"
#include "roc_core/macro_helpers.h"

#include <sndfile.h>

namespace roc {
namespace sndio {

namespace {

// These constants are not defined in older versions of libsndfile,
// so we define them explicitly. If libsndfile doesn't actually
// support them, we'll detect it at run-time.
enum {
    // ogg
    SF_FORMAT_OGG = 0x200000,
    SF_FORMAT_VORBIS = 0x0060,
    SF_FORMAT_OPUS = 0x0064,
    // mpeg
    SF_FORMAT_MPEG = 0x230000,
    SF_FORMAT_MPEG_LAYER_I = 0x0080,
    SF_FORMAT_MPEG_LAYER_II = 0x0081,
    SF_FORMAT_MPEG_LAYER_III = 0x0082,
};

} // namespace

SndfileFormatInfo sndfile_format_remap[ROC_ARRAY_SIZE(sndfile_format_remap)] = {
    { "pcm", NULL, SF_FORMAT_RAW },
    { "ogg", ".ogg", SF_FORMAT_OGG },
    { "mp1", ".mp1", SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_I },
    { "mp2", ".mp2", SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_II },
    { "mp3", ".mp3", SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III },
    { "mat", ".mat", SF_FORMAT_MAT5 },
    { "wav", ".wav", SF_FORMAT_WAV },
    { "nist", NULL, SF_FORMAT_NIST },
    { "wavex", NULL, SF_FORMAT_WAVEX },
};

SndfileSubformatInfo sndfile_subformat_map[ROC_ARRAY_SIZE(sndfile_subformat_map)] = {
    // lpcm
    { "lpcm", "ulaw", SF_FORMAT_ULAW },
    { "lpcm", "alaw", SF_FORMAT_ALAW },
    // dpcm
    { "dpcm", "d8", SF_FORMAT_DPCM_8 },
    { "dpcm", "d16", SF_FORMAT_DPCM_16 },
    // adpcm
    { "adpcm", "adpcm_ima", SF_FORMAT_IMA_ADPCM },
    { "adpcm", "adpcm_ms", SF_FORMAT_MS_ADPCM },
    { "adpcm", "adpcm_vox", SF_FORMAT_VOX_ADPCM },
    // dwvw
    { "dwvw", "dwvw12", SF_FORMAT_DWVW_12 },
    { "dwvw", "dwvw16", SF_FORMAT_DWVW_16 },
    { "dwvw", "dwvw24", SF_FORMAT_DWVW_24 },
    // g72x
    { "g72x", "g721_32", SF_FORMAT_G721_32 },
    { "g72x", "g723_24", SF_FORMAT_G723_24 },
    { "g72x", "g723_40", SF_FORMAT_G723_40 },
    // ogg
    { "ogg", "vorbis", SF_FORMAT_VORBIS },
    { "ogg", "opus", SF_FORMAT_OPUS },
};

int sndfile_explicit_formats[ROC_ARRAY_SIZE(sndfile_explicit_formats)] = {
    SF_FORMAT_RAW, // pcm
};

int sndfile_default_subformats[ROC_ARRAY_SIZE(sndfile_default_subformats)] = {
    // most major formats supports at least one PCM or DPCM sub-format
    SF_FORMAT_PCM_24,
    SF_FORMAT_PCM_16,
    SF_FORMAT_DPCM_16,
    // for caf
    SF_FORMAT_ULAW,
    SF_FORMAT_ALAW,
    // for ogg
    SF_FORMAT_VORBIS,
    SF_FORMAT_OPUS,
};

} // namespace sndio
} // namespace roc
