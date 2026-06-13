/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "roc_core/time.h"

namespace roc {
namespace helper {
namespace {

enum { WarmupIterations = 10 };

double round_digits(double x, unsigned int digits) {
    double fac = pow(10, digits);
    return round(x * fac) / fac;
}

void busy_wait(core::nanoseconds_t delay) {
    const core::nanoseconds_t deadline = core::timestamp(core::ClockMonotonic) + delay;
    for (;;) {
        if (core::timestamp(core::ClockMonotonic) >= deadline) {
            return;
        }
    }
}

class Counter {
private:
    enum { NumBuckets = 500 };

public:
    Counter()
        : last_(0)
        , total_(0)
        , count_(0)
        , warmed_up_(false) {
        memset(buckets_, 0, sizeof(buckets_));
    }

    void begin() {
        last_ = core::timestamp(core::ClockMonotonic);
    }

    void end() {
        add_time(core::timestamp(core::ClockMonotonic) - last_);
    }

    void add_time(core::nanoseconds_t t) {
        if (count_ == WarmupIterations && !warmed_up_) {
            *this = Counter();
            warmed_up_ = true;
        }

        total_ += t;
        count_++;

        for (int n = NumBuckets; n > 0; n--) {
            if (t <= core::Microsecond * 10 * (n + 1)) {
                buckets_[n]++;
            } else {
                break;
            }
        }
    }

    double avg() const {
        return round_digits(double(total_) / count_ / 1000, 3);
    }

    double p95() const {
        for (int n = 0; n < NumBuckets; n++) {
            const double ratio = double(buckets_[n]) / count_;
            if (ratio >= 0.95) {
                return 10 * (n + 1);
            }
        }
        return 1. / 0.;
    }

    size_t count() const {
        return count_;
    }

private:
    core::nanoseconds_t last_;

    core::nanoseconds_t total_;
    size_t count_;

    core::nanoseconds_t buckets_[NumBuckets];

    bool warmed_up_;
};

} // namespace
} // namespace helper
} // namespace roc