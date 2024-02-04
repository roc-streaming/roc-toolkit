/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/version.h"

void roc_version_load(roc_version* version) {
    if (!version) {
        return;
    }

    version->major = ROC_VERSION_MAJOR;
    version->minor = ROC_VERSION_MINOR;
    version->patch = ROC_VERSION_PATCH;

    version->code = ROC_VERSION;
}
