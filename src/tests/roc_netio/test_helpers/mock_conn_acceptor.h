/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_HELPERS_MOCK_CONN_ACCEPTOR_H_
#define ROC_NETIO_TEST_HELPERS_MOCK_CONN_ACCEPTOR_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_netio/iconn_acceptor.h"

namespace roc {
namespace netio {
namespace test {

class MockConnAcceptor : public IConnAcceptor {
public:
    explicit MockConnAcceptor()
        : cond_(mutex_)
        , next_returned_handler_(0)
        , num_handlers_(0)
        , num_alive_handlers_(0)
        , added_conn_(NULL)
        , removed_conn_handler_(NULL)
        , drop_next_conn_(false)
        , add_calls_(0)
        , remove_calls_(0)
        , wait_added_calls_(0)
        , wait_removed_calls_(0) {
    }

    virtual ~MockConnAcceptor() {
        // all pushed handlers were used
        roc_panic_if(next_returned_handler_ != num_handlers_);

        // all added handler were removed
        roc_panic_if(num_alive_handlers_ != 0);
        roc_panic_if(add_calls_ != remove_calls_);

        // all adds and removes were waited
        roc_panic_if(add_calls_ != wait_added_calls_);
        roc_panic_if(remove_calls_ != wait_removed_calls_);

        // all drop commands were processed
        roc_panic_if(drop_next_conn_);
    }

    virtual IConnHandler* add_connection(IConn& conn) {
        core::Mutex::Lock lock(mutex_);

        if (drop_next_conn_) {
            drop_next_conn_ = false;
            return NULL;
        }

        roc_panic_if(next_returned_handler_ == num_handlers_);
        roc_panic_if(added_conn_);

        added_conn_ = &conn;
        cond_.broadcast();

        add_calls_++;
        num_alive_handlers_++;

        return handlers_[next_returned_handler_++];
    }

    virtual void remove_connection(IConnHandler& handler) {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if_not(num_alive_handlers_ > 0);

        size_t pos = 0;
        for (; pos < num_handlers_; pos++) {
            if (handlers_[pos] == &handler) {
                break;
            }
        }

        roc_panic_if_not(pos < num_handlers_);
        roc_panic_if_not(pos < next_returned_handler_);

        roc_panic_if(removed_conn_handler_);

        removed_conn_handler_ = &handler;
        cond_.broadcast();

        remove_calls_++;
        num_alive_handlers_--;
    }

    void push_handler(IConnHandler& handler) {
        core::Mutex::Lock lock(mutex_);

        CHECK(num_handlers_ < MaxHandlers - 1);
        handlers_[num_handlers_++] = &handler;
    }

    void drop_next_connection() {
        core::Mutex::Lock lock(mutex_);

        roc_panic_if(drop_next_conn_);

        drop_next_conn_ = true;
    }

    IConn* wait_added() {
        core::Mutex::Lock lock(mutex_);

        wait_added_calls_++;

        while (added_conn_ == NULL) {
            cond_.wait();
        }

        IConn* conn = added_conn_;
        added_conn_ = NULL;

        CHECK(conn);
        return conn;
    }

    IConnHandler* wait_removed() {
        core::Mutex::Lock lock(mutex_);

        wait_removed_calls_++;

        while (removed_conn_handler_ == NULL) {
            cond_.wait();
        }

        IConnHandler* conn_handler = removed_conn_handler_;
        removed_conn_handler_ = NULL;

        CHECK(conn_handler);
        return conn_handler;
    }

private:
    enum { MaxHandlers = 10 };

    core::Mutex mutex_;
    core::Cond cond_;

    IConnHandler* handlers_[MaxHandlers];
    size_t next_returned_handler_;
    size_t num_handlers_;
    size_t num_alive_handlers_;

    IConn* added_conn_;
    IConnHandler* removed_conn_handler_;
    bool drop_next_conn_;

    size_t add_calls_;
    size_t remove_calls_;

    size_t wait_added_calls_;
    size_t wait_removed_calls_;
};

} // namespace test
} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TEST_HELPERS_MOCK_CONN_ACCEPTOR_H_
