/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/memory_limiter.h"

#include "roc_core/cpu_instructions.h"
#include "roc_core/log.h"

namespace roc {
namespace core {

MemoryLimiter::MemoryLimiter(const char* name, size_t max_bytes)
    : name_(name)
    , max_bytes_(max_bytes)
    , bytes_acquired_(0) {
}

MemoryLimiter::~MemoryLimiter() {
    if (bytes_acquired_ > 0) {
        roc_panic("memory limiter (%s): detected that memory has not been released: "
                  "acquired=%lu",
                  name_, (unsigned long)bytes_acquired_);
    }
}

bool MemoryLimiter::acquire(size_t num_bytes) {
    if (num_bytes == 0) {
        roc_panic("memory limiter (%s): tried to acquire zero bytes", name_);
    }
    size_t current;
    do {
        current = bytes_acquired_;
        size_t next = current + num_bytes;
        if (max_bytes_ > 0 && next > max_bytes_) {
            break;
        }
        if (bytes_acquired_.compare_exchange(current, next)) {
            return true;
        }
        cpu_relax();
    } while (true);
    roc_log(LogError,
            "memory limiter (%s): could not acquire bytes due to limit: requested=%lu "
            "acquired=%lu limit=%lu",
            name_, (unsigned long)num_bytes, (unsigned long)current,
            (unsigned long)max_bytes_);
    return false;
}

void MemoryLimiter::release(size_t num_bytes) {
    if (num_bytes == 0) {
        roc_panic("memory limiter (%s): tried to release zero bytes", name_);
    }
    size_t next = bytes_acquired_ -= num_bytes;
    size_t prev = next + num_bytes;
    if (next > prev) {
        roc_panic("memory limiter (%s): tried to release too many bytes: requested=%lu, "
                  "acquired=%lu",
                  name_, (unsigned long)num_bytes, (unsigned long)prev);
    }
}

size_t MemoryLimiter::num_acquired() {
    return bytes_acquired_;
}

} // namespace core
} // namespace roc
