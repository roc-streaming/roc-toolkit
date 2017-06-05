/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/fs.h"
#include "roc_core/log.h"

namespace roc {
namespace core {

bool create_temp_dir(char* path, size_t path_sz) {
    const char* dir = getenv("TMPDIR");
    if (!dir) {
        dir = P_tmpdir;
    }
    if (!dir) {
        dir = "/tmp";
    }
    if (snprintf(path, path_sz, "%s/XXXXXX", dir) < 0) {
        roc_log(LogError, "snprintf(): %s/XXXXXX: %s", dir, errno_to_str().c_str());
        return false;
    }
    if (!mkdtemp(path)) {
        roc_log(LogInfo, "mkstemp(): %s: %s", path, errno_to_str().c_str());
        return false;
    }
    return true;
}

bool remove_dir(const char* path) {
    int ret = rmdir(path);
    if (ret != 0) {
        roc_log(LogInfo, "rmdir(): %s: %s", path, errno_to_str().c_str());
    }
    return (ret == 0);
}

} // namespace core
} // namespace roc
