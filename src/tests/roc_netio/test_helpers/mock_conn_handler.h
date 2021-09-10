/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_HELPERS_MOCK_CONN_HANDLER_H_
#define ROC_NETIO_TEST_HELPERS_MOCK_CONN_HANDLER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/cond.h"
#include "roc_core/log.h"
#include "roc_core/mutex.h"
#include "roc_netio/iconn_handler.h"

#include "test_helpers/conn_expectation.h"

namespace roc {
namespace netio {
namespace test {

class MockConnHandler : public IConnHandler {
public:
    MockConnHandler()
        : cond_(mutex_)
        , conn_(NULL)
        , refused_(false)
        , established_(false)
        , writable_(false)
        , readable_(false)
        , terminated_(false)
        , failed_at_terminate_(false)
        , wait_refused_called_(false)
        , wait_established_called_(false)
        , wait_terminated_called_(false) {
    }

    virtual ~MockConnHandler() {
        roc_panic_if_not(wait_refused_called_ || wait_established_called_);
        roc_panic_if_not(wait_terminated_called_);
    }

    virtual void connection_refused(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if(conn_ != NULL);
        roc_panic_if(established_ || refused_);

        roc_panic_if_not(conn.local_address().has_host_port());
        roc_panic_if_not(conn.remote_address().has_host_port());

        roc_panic_if_not(conn.is_failed());
        roc_panic_if_not(!conn.is_writable());
        roc_panic_if_not(!conn.is_readable());

        conn_ = &conn;
        refused_ = true;

        cond_.broadcast();
    }

    virtual void connection_established(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if(conn_ != NULL);
        roc_panic_if(established_ || refused_);

        roc_panic_if_not(conn.local_address().has_host_port());
        roc_panic_if_not(conn.remote_address().has_host_port());

        roc_panic_if_not(!conn.is_failed());

        conn_ = &conn;
        established_ = true;

        cond_.broadcast();
    }

    virtual void connection_writable(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if_not(conn_ == &conn);
        roc_panic_if_not(established_);

        conn_ = &conn;
        writable_ = true;

        cond_.broadcast();
    }

    virtual void connection_readable(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if_not(conn_ == &conn);
        roc_panic_if_not(established_);

        conn_ = &conn;
        readable_ = true;

        cond_.broadcast();
    }

    virtual void connection_terminated(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if_not(conn_ == &conn);
        roc_panic_if_not(refused_ || established_);

        roc_panic_if_not(conn.local_address().has_host_port());
        roc_panic_if_not(conn.remote_address().has_host_port());

        roc_panic_if_not(!conn.is_writable());
        roc_panic_if_not(!conn.is_readable());

        conn_ = &conn;
        terminated_ = true;
        failed_at_terminate_ = conn.is_failed();

        cond_.broadcast();
    }

    IConn* wait_refused() {
        roc_log(LogInfo, "mock handler: wait_refused() begin");

        core::Mutex::Lock lock(mutex_);

        wait_refused_called_ = true;

        while (!refused_ && !established_) {
            cond_.wait();
        }

        CHECK(refused_);
        CHECK(!established_);

        CHECK(conn_);

        roc_log(LogInfo, "mock handler: wait_refused() end");
        return conn_;
    }

    IConn* wait_established() {
        roc_log(LogInfo, "mock handler: wait_established() begin");

        core::Mutex::Lock lock(mutex_);

        wait_established_called_ = true;

        while (!refused_ && !established_) {
            cond_.wait();
        }

        CHECK(established_);
        CHECK(!refused_);

        CHECK(conn_);

        roc_log(LogInfo, "mock handler: wait_established() end");
        return conn_;
    }

    void wait_writable() {
        roc_log(LogInfo, "mock handler: wait_writable() begin");

        core::Mutex::Lock lock(mutex_);

        while (!writable_) {
            cond_.wait();
        }

        writable_ = false;

        CHECK(conn_);
        roc_log(LogInfo, "mock handler: wait_writable() end");
    }

    void wait_readable() {
        roc_log(LogInfo, "mock handler: wait_readable() begin");

        core::Mutex::Lock lock(mutex_);

        while (!readable_) {
            cond_.wait();
        }

        readable_ = false;

        CHECK(conn_);
        roc_log(LogInfo, "mock handler: wait_readable() end");
    }

    void wait_terminated(ConnExpectation exp) {
        roc_log(LogInfo, "mock handler: wait_terminated() begin");

        core::Mutex::Lock lock(mutex_);

        wait_terminated_called_ = true;

        while (!terminated_) {
            cond_.wait();
        }

        CHECK(conn_);
        if (exp == ExpectNotFailed) {
            CHECK(!failed_at_terminate_);
        } else {
            CHECK(failed_at_terminate_);
        }
        roc_log(LogInfo, "mock handler: wait_terminated() end");
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    IConn* conn_;

    bool refused_;
    bool established_;
    bool writable_;
    bool readable_;
    bool terminated_;
    bool failed_at_terminate_;

    bool wait_refused_called_;
    bool wait_established_called_;
    bool wait_terminated_called_;
};

} // namespace test
} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TEST_HELPERS_MOCK_CONN_HANDLER_H_
