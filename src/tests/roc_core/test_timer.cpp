/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_core/timer.h"

namespace roc {
namespace core {

namespace {

class TestThread : public core::Thread {
public:
    TestThread(Timer& t)
        : t_(t)
        , r_(0) {
    }

    bool running() const {
        return r_;
    }

    void wait_running() {
        while (!r_) {
            sleep_for(core::Microsecond);
        }
    }

private:
    virtual void run() {
        r_ = true;
        t_.wait_deadline();
        r_ = false;
    }

    Timer& t_;
    Atomic<int> r_;
};

inline void set_deadline(Timer& t, nanoseconds_t delay) {
    if (!t.try_set_deadline(delay > 0 ? timestamp() + delay : delay)) {
        FAIL("try_set_deadline");
    }
}

} // namespace

TEST_GROUP(timer) {};

TEST(timer, sync) {
    { // default
        Timer t;
        t.wait_deadline();
    }
    { // explicit zero
        Timer t;
        set_deadline(t, core::Second * 100);
        set_deadline(t, 0);
        t.wait_deadline();
    }
    { // multiple times
        Timer t;
        t.wait_deadline();
        t.wait_deadline();
        t.wait_deadline();
    }
    { // non-zero
        Timer t;
        set_deadline(t, core::Microsecond * 100);
        t.wait_deadline();
    }
}

TEST(timer, async) {
    { // infinity -> zero
        Timer t;
        set_deadline(t, -1);

        TestThread thr(t);
        CHECK(thr.start());

        thr.wait_running();
        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, 0);
        thr.join();
    }
    { // large -> small
        Timer t;
        set_deadline(t, core::Second * 999);

        TestThread thr(t);
        CHECK(thr.start());

        thr.wait_running();
        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, core::Microsecond * 10);
        thr.join();
    }
    { // large -> smaller -> small
        Timer t;
        set_deadline(t, core::Second * 999);

        TestThread thr(t);
        CHECK(thr.start());

        thr.wait_running();
        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, core::Second * 99);

        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, core::Microsecond * 10);
        thr.join();
    }
    { // large -> larger -> small
        Timer t;
        set_deadline(t, core::Second * 999);

        TestThread thr(t);
        CHECK(thr.start());

        thr.wait_running();
        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, core::Second * 99999);

        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, core::Microsecond * 10);
        thr.join();
    }
    { // duplicate
        Timer t;
        set_deadline(t, -1);

        TestThread thr(t);
        CHECK(thr.start());

        thr.wait_running();
        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, -1);
        set_deadline(t, -1);
        set_deadline(t, -1);

        sleep_for(core::Microsecond * 100);
        CHECK(thr.running());

        set_deadline(t, 0);
        thr.join();
    }
    { // repeat
        Timer t;

        for (int i = 0; i < 3; i++) {
            set_deadline(t, -1);

            TestThread thr(t);
            CHECK(thr.start());

            thr.wait_running();
            sleep_for(core::Microsecond * 100);
            CHECK(thr.running());

            set_deadline(t, 0);
            thr.join();
        }
    }
}

} // namespace core
} // namespace roc
