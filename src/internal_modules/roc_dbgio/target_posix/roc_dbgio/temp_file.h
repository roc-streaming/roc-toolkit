/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_dbgio/target_posix/roc_dbgio/temp_file.h
//! @brief Filesystem functions.

#ifndef ROC_DBGIO_TEMP_FILE_H_
#define ROC_DBGIO_TEMP_FILE_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace dbgio {

//! Temporary file.
//! @remarks
//!  A temporary file is created in constructor and removed in destructor.
class TempFile : public core::NonCopyable<> {
public:
    //! Create temporary file.
    //! @remarks
    //!  Creates a temporary directory and a file with given @p name inside it.
    //!  Both will be removed in destructor.
    TempFile(const char* name);

    ~TempFile();

    //! Get file path.
    const char* path() const;

private:
    char dir_[1024];
    char file_[1024];
};

} // namespace dbgio
} // namespace roc

#endif // ROC_DBGIO_TEMP_FILE_H_
