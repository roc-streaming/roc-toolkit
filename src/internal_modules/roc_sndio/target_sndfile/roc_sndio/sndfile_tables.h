/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_tables.h
//! @brief Sndfile tables.

#ifndef ROC_SNDIO_SNDFILE_TABLES_H_
#define ROC_SNDIO_SNDFILE_TABLES_H_

namespace roc {
namespace sndio {

//! Sndfile format meta-data.
struct SndfileFormatInfo {
    //! Name of the format.
    const char* name;
    //! File extension associated with the format.
    const char* file_extension;
    //! SF_FORMAT corresponding to the driver.
    int format_mask;
};

//! Sndfile driver meta-data.
struct SndfileSubformatInfo {
    //! Name of sub-format group.
    const char* group;
    //! Name of sub-format.
    const char* name;
    //! SF_FORMAT corresponding to the sub-format.
    int format_mask;
};

//! Table of sndfile formats with re-mapped names or file extensions.
//! This table is checked when user explicitly specifies format name,
//! or we're trying to guess format from file extension.
extern SndfileFormatInfo sndfile_format_remap[9];

//! Table of sndfile sub-formats with mapped string names and divided into groups.
//! This table is checked when user explicitly specifies sub-format name.
extern SndfileSubformatInfo sndfile_subformat_map[15];

//! Table of sndfile formats which require explicitly providing sub-format,
//! rate, and channels.
extern int sndfile_explicit_formats[1];

//! Table of sndfile sub-formats to try when no specific sub-format requested.
//! This list provides the minimum number of sub-formats needed to support
//! all possible major formats.
extern int sndfile_default_subformats[7];

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_TABLES_H_
