/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_extension_table.h
//! @brief Sndfile driver map.

#ifndef ROC_SNDIO_SNDFILE_EXTENSION_TABLE_H_
#define ROC_SNDIO_SNDFILE_EXTENSION_TABLE_H_

namespace roc {
namespace sndio {

//! Sndfile driver map.
struct FileMap {
    //! SF_FORMAT ID corresponding to the enum value in sndfile.h
    int format_id;
    //! Name of driver mapped to SF_FORMAT
    const char* driver_name;
    //! File extension associated with driver and SF_FORMAT if it exists.
    const char* file_extension;
};

//! Declare the file_type_map as extern
extern FileMap file_type_map[5];

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_EXTENSION_TABLE_H_
