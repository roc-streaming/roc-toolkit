/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

using namespace roc;

void roc_version(int* major, int* minor, int* patch) {
    if (major) *major = ROC_VERSION_MAJOR;
    if (minor) *minor = ROC_VERSION_MINOR;
    if (patch) *patch = ROC_VERSION_PATCH;
}