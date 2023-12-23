/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/identity.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace rtp {

Identity::Identity()
    : valid_(false) {
    if (!core::uuid_generare(cname_, sizeof(cname_))) {
        return;
    }

    if (!change_ssrc()) {
        return;
    }

    valid_ = true;
}

bool Identity::is_valid() const {
    return valid_;
}

const char* Identity::cname() const {
    roc_panic_if(!is_valid());

    return cname_;
}

packet::stream_source_t Identity::ssrc() const {
    roc_panic_if(!is_valid());

    return ssrc_;
}

bool Identity::change_ssrc() {
    ssrc_ =
        (packet::stream_source_t)core::fast_random_range(1, packet::stream_source_t(-1));

    roc_log(LogDebug, "rtp identity: ssrc=%lu cname=%s", (unsigned long)ssrc_, cname_);

    return true;
}

} // namespace rtp
} // namespace roc
