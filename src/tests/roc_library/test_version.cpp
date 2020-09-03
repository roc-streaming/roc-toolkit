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
namespace library {

TEST_GROUP(version) {};

TEST(version, version_num) {
    roc_version version = { 0, 0, 0 };
    roc_get_version(&version);

    UNSIGNED_LONGS_EQUAL(ROC_VERSION_MAJOR, version.major);
    UNSIGNED_LONGS_EQUAL(ROC_VERSION_MINOR, version.minor);
    UNSIGNED_LONGS_EQUAL(ROC_VERSION_PATCH, version.patch);
}

} // namespace library
} // namespace roc
