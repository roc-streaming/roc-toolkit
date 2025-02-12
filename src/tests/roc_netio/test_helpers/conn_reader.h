/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_HELPERS_CONN_READER_H_
#define ROC_NETIO_TEST_HELPERS_CONN_READER_H_

#include "roc_core/fast_random.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_netio/iconn.h"

#include "test_helpers/mock_conn_handler.h"

#include <CppUTest/TestHarness.h>

namespace roc {
namespace netio {
namespace test {

class ConnReader : public core::Thread {
public:
    ConnReader(MockConnHandler& handler, IConn& conn, size_t total_bytes)
        : handler_(handler)
        , conn_(conn)
        , total_bytes_(total_bytes) {
    }

private:
    virtual void run() {
        enum { MaxBatch = 1024 };

        uint8_t current_byte = 0;
        size_t num_read = 0;

        while (num_read < total_bytes_) {
            roc_panic_if(conn_.is_failed());

            handler_.wait_readable();

            while (num_read < total_bytes_) {
                size_t bufsz = core::fast_random_range(1, MaxBatch);
                if (bufsz > (total_bytes_ - num_read)) {
                    bufsz = (total_bytes_ - num_read);
                }

                uint8_t buf[MaxBatch];
                memset(buf, 0, sizeof(buf));

                const ssize_t ret = conn_.try_read(buf, bufsz);

                if (ret == SockErr_WouldBlock) {
                    break;
                } else {
                    if (ret < 1 || ret > (ssize_t)bufsz) {
                        roc_panic(
                            "conn reader: try_read() returned %ld, expected [1; %ld]",
                            (long)ret, (long)bufsz);
                    }
                }

                for (size_t i = 0; i < (size_t)ret; i++) {
                    roc_panic_if_not(buf[i] == current_byte);
                    current_byte++;
                }

                num_read += (size_t)ret;
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

#endif // ROC_NETIO_TEST_HELPERS_CONN_READER_H_
