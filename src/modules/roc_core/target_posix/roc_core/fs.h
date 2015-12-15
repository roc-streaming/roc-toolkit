/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/fs.h
//! @brief Filesystem functions.

#ifndef ROC_CORE_FS_H_
#define ROC_CORE_FS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Create temporary directory.
//! @remarks
//!  Writes generated absolute path to @p path.
//! @returns
//!  false if directory can't be created or @p path_sz is too small.
bool create_temp_dir(char* path, size_t path_sz);

//! Remove empty directory.
//! @returns
//!  false if directory is non-empty or can't be removed.
bool remove_dir(const char* path);

} // namespace core
} // namespace roc

#endif // ROC_CORE_FS_H_
