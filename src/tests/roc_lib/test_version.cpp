/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc/version.h"

namespace roc {

TEST_GROUP(version) {};

TEST(version, version_num) {
    int major, minor, patch;
    roc_version(&major, &minor, &patch);

    LONGS_EQUAL(ROC_VERSION_MAJOR, major);
    LONGS_EQUAL(ROC_VERSION_MINOR, minor);
    LONGS_EQUAL(ROC_VERSION_PATCH, patch);
}

} // namespace roc