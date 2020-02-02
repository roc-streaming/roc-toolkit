/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_sdp/session_description.h"

namespace roc {
namespace sdp {

TEST_GROUP(sdp_parser) {
    core::HeapAllocator allocator;
};

TEST(sdp_parser, str) {
    SessionDescription session_description(allocator);
    CHECK(parse_sdp("v=0\n"
                    "o=test_origin 16914 1 IN IP4 192.168.58.15\n"
                    "c=IN IP4 239.255.12.42/255\n"
                    "m=audio 12345 RTP/AVP 10",
                    session_description));
}

} // namespace sdp
} // namespace roc
