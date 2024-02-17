/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/uuid.h"

namespace roc {
namespace core {

TEST_GROUP(uuid) {};

TEST(uuid, generate) {
    char a_uuid[UuidLen + 1] = {};

    CHECK(uuid_generare(a_uuid, sizeof(a_uuid)) == true);
    CHECK(a_uuid[8] == '-');
    CHECK(a_uuid[13] == '-');
    CHECK(a_uuid[18] == '-');
    CHECK(a_uuid[23] == '-');
    CHECK(a_uuid[UuidLen] == '\0');
}

TEST(uuid, generated_with_bigger_buffer) {
    char a_uuid[UuidLen + 1 + 4] = {};

    CHECK(uuid_generare(a_uuid, sizeof(a_uuid)) == true);
    CHECK(a_uuid[8] == '-');
    CHECK(a_uuid[13] == '-');
    CHECK(a_uuid[18] == '-');
    CHECK(a_uuid[23] == '-');
    CHECK(a_uuid[UuidLen] == '\0');
}

} // namespace core
} // namespace roc
