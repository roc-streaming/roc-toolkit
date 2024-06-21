/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_FEC_TEST_HELPERS_STATUS_WRITER_H_
#define ROC_FEC_TEST_HELPERS_STATUS_WRITER_H_

#include "roc_packet/iwriter.h"
#include "roc_status/status_code.h"

namespace roc {
namespace fec {
namespace test {

class StatusWriter : public packet::IWriter {
public:
    explicit StatusWriter(status::StatusCode code)
        : code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&) {
        return code_;
    }

private:
    status::StatusCode code_;
};

} // namespace test
} // namespace fec
} // namespace roc

#endif // ROC_FEC_TEST_HELPERS_STATUS_WRITER_H_
