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

//! Sndfile driver meta-data.
struct SndfileDriverInfo {
    //! Name of the driver.
    const char* driver_name;
    //! File extension associated with driver.
    const char* file_extension;
    //! SF_FORMAT corresponding to the driver.
    int format_mask;
};

//! Table of sndfile drivers with re-mapped names or file extensions.
//! This table should be checked when we need to guess format from driver
//! name or file extension.
extern SndfileDriverInfo sndfile_driver_remap[9];

//! Table of sndfile sub-formats to try when no specific format requested.
//! This list provides the minimum number of sub-formats needed to support
//! all possible major formats.
extern int sndfile_default_subformats[7];

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_TABLES_H_
