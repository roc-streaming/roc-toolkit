/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_HELPERS_CONN_WRITER_H_
#define ROC_NETIO_TEST_HELPERS_CONN_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/fast_random.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_netio/iconn.h"

#include "test_helpers/mock_conn_handler.h"

namespace roc {
namespace netio {
namespace test {

class ConnWriter : public core::Thread {
public:
    ConnWriter(MockConnHandler& handler, IConn& conn, size_t total_bytes)
        : handler_(handler)
        , conn_(conn)
        , total_bytes_(total_bytes) {
    }

private:
    virtual void run() {
        enum { MaxBatch = 1024, MaxDelayNs = 1000 };

        uint32_t current_byte = 0;
        size_t num_written = 0;

        while (num_written < total_bytes_) {
            roc_panic_if(conn_.is_failed());

            handler_.wait_writable();

            while (num_written < total_bytes_) {
                const core::nanoseconds_t delay =
                    (core::nanoseconds_t)core::fast_random_range(
                        0, MaxDelayNs * core::Nanosecond);

                core::sleep_for(core::ClockMonotonic, delay);

                size_t bufsz = core::fast_random_range(1, MaxBatch);
                if (bufsz > (total_bytes_ - num_written)) {
                    bufsz = (total_bytes_ - num_written);
                }

                uint8_t buf[MaxBatch];
                for (size_t i = 0; i < bufsz; i++) {
                    buf[i] = (uint8_t)(current_byte + i);
                }

                const ssize_t ret = conn_.try_write(buf, bufsz);

                if (ret == IOErr_WouldBlock) {
                    break;
                } else {
                    if (ret < 1 || ret > (ssize_t)bufsz) {
                        roc_panic(
                            "conn reader: try_write() returned %ld, expected [1; %ld]",
                            (long)ret, (long)bufsz);
                    }
                }

                current_byte += (size_t)ret;
                num_written += (size_t)ret;
            }
        }
    }

    MockConnHandler& handler_;
    IConn& conn_;

    const size_t total_bytes_;
};

} // namespace test
} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TEST_HELPERS_CONN_WRITER_H_
