/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_RTP_TEST_HELPERS_STATUS_READER_H_
#define ROC_RTP_TEST_HELPERS_STATUS_READER_H_

#include "roc_packet/ireader.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {
namespace test {

class StatusReader : public packet::IReader {
public:
    explicit StatusReader(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& pp,
                                                       packet::PacketReadMode mode) {
        return code_;
    }

private:
    status::StatusCode code_;
};

} // namespace test
} // namespace rtp
} // namespace roc

#endif // ROC_RTP_TEST_HELPERS_STATUS_READER_H_
