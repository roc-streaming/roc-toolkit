/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/identity.h"
#include "roc_core/log.h"
#include "roc_core/secure_random.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

Identity::Identity()
    : init_status_(status::NoStatus) {
    if (!core::uuid_generate(cname_, sizeof(cname_))) {
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
    bool ok = (packet::stream_source_t)core::secure_random_range_32(
        1, packet::stream_source_t(-1), ssrc_);
    if (!ok) {
        return status::StatusErrRand;
    }
    roc_log(LogDebug, "rtp identity: ssrc=%lu cname=%s", (unsigned long)ssrc_, cname_);

    return status::StatusOK;
}

} // namespace rtp
} // namespace roc
