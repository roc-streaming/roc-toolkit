/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/refcnt.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

namespace {

class LeakDetector {
public:
    LeakDetector()
        : counter_(0)
        , enabled_(0) {
    }

    ~LeakDetector() {
        if (enabled_ && counter_ != 0) {
            roc_panic("reference countable objects leak detected: n_leaked_objects=%d",
                      (int)counter_);
        }
    }

    void enable() {
        enabled_ = true;
    }

    void incref() {
        ++counter_;
    }

    void decref() {
        roc_panic_if(counter_ == 0);
        --counter_;
    }

private:
    Atomic counter_;
    Atomic enabled_;
};

LeakDetector g_leak_detector;

} // namespace

void RefCnt::enable_leak_detection() {
    g_leak_detector.enable();
}

RefCnt::RefCnt()
    : counter_(0) {
    g_leak_detector.incref();
}

RefCnt::~RefCnt() {
    if (counter_ != 0) {
        roc_panic("reference counter is non-zero in destructor (counter = %d)",
                  (int)counter_);
    }

    counter_ = -1;

    g_leak_detector.decref();
}

} // namespace core
} // namespace roc
