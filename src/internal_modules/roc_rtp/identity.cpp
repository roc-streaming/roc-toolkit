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
    : init_status_(status::NoStatus) {
    if (!core::uuid_generare(cname_, sizeof(cname_))) {
        init_status_ = status::StatusErrRand;
        return;
    }

    if ((init_status_ = change_ssrc()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Identity::init_status() const {
    return init_status_;
}

const char* Identity::cname() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return cname_;
}

packet::stream_source_t Identity::ssrc() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return ssrc_;
}

status::StatusCode Identity::change_ssrc() {
    ssrc_ =
        (packet::stream_source_t)core::fast_random_range(1, packet::stream_source_t(-1));

    roc_log(LogDebug, "rtp identity: ssrc=%lu cname=%s", (unsigned long)ssrc_, cname_);

    return status::StatusOK;
}

} // namespace rtp
} // namespace roc
