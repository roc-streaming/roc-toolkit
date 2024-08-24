/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_dbgio/temp_file.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace roc {
namespace dbgio {

TempFile::TempFile(const char* name) {
    dir_[0] = '\0';
    file_[0] = '\0';

    const char* tempdir = getenv("TMPDIR");
    if (!tempdir) {
        tempdir = P_tmpdir;
    }
    if (!tempdir) {
        tempdir = "/tmp";
    }

    if (snprintf(dir_, sizeof(dir_), "%s/roc-XXXXXX", tempdir) < 0) {
        roc_log(LogError, "temp file: snprintf(): %s", core::errno_to_str().c_str());
        return;
    }

    if (mkdtemp(dir_) == NULL) {
        roc_log(LogError, "temp file: mkdtemp(): %s: %s", dir_,
                core::errno_to_str().c_str());
        return;
    }

    if (snprintf(file_, sizeof(file_), "%s/%s", dir_, name) < 0) {
        roc_log(LogError, "temp file: snprintf(): %s", core::errno_to_str().c_str());
        return;
    }

    int fd = open(file_, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
    if (fd == -1) {
        roc_log(LogError, "temp file: open(): %s: %s", file_,
                core::errno_to_str().c_str());
        return;
    }

    if (close(fd) == -1) {
        roc_log(LogError, "temp file: close(): %s: %s", file_,
                core::errno_to_str().c_str());
    }

    roc_log(LogDebug, "temp file: created %s", file_);
}

TempFile::~TempFile() {
    if (*file_) {
        roc_log(LogDebug, "temp file: removing %s", file_);

        if (unlink(file_) == -1) {
            roc_log(LogError, "temp file: unlink(): %s: %s", file_,
                    core::errno_to_str().c_str());
        }
    }

    if (*dir_) {
        if (rmdir(dir_) == -1) {
            roc_log(LogError, "temp file: rmdir(): %s: %s", dir_,
                    core::errno_to_str().c_str());
        }
    }
}

const char* TempFile::path() const {
    return file_;
}

} // namespace dbgio
} // namespace roc
