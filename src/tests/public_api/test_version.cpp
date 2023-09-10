/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc/version.h"

namespace roc {
namespace api {

TEST_GROUP(version) {};

TEST(version, null) {
    roc_version_get(NULL);
}

TEST(version, get) {
    roc_version version;
    roc_version_get(&version);

    UNSIGNED_LONGS_EQUAL(ROC_VERSION_MAJOR, version.major);
    UNSIGNED_LONGS_EQUAL(ROC_VERSION_MINOR, version.minor);
    UNSIGNED_LONGS_EQUAL(ROC_VERSION_PATCH, version.patch);

    UNSIGNED_LONGS_EQUAL(ROC_VERSION, version.code);
    UNSIGNED_LONGS_EQUAL(ROC_VERSION_CODE(version.major, version.minor, version.patch),
                         version.code);
}

TEST(version, code) {
    CHECK(ROC_VERSION_CODE(1, 2, 3) == ROC_VERSION_CODE(1, 2, 3));
    CHECK(ROC_VERSION_CODE(1, 2, 3) < ROC_VERSION_CODE(1, 2, 4));
    CHECK(ROC_VERSION_CODE(1, 2, 3) < ROC_VERSION_CODE(1, 3, 0));
    CHECK(ROC_VERSION_CODE(1, 2, 3) < ROC_VERSION_CODE(2, 0, 0));
}

} // namespace api
} // namespace roc
