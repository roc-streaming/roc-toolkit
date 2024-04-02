/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/csv_dumper.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

CsvDumper::CsvDumper(const char* path, const CsvConfig& config, IArena& arena)
    : config_(config)
    , ringbuf_(arena, config.max_queued)
    , valid_(false) {
    if (!open_(path)) {
        return;
    }
    valid_ = true;
}

CsvDumper::~CsvDumper() {
    if (is_joinable()) {
        roc_panic("csv dumper: attempt to call destructor"
                  " before calling stop() and join()");
    }

    close_();
}

void CsvDumper::write(const CsvEntry& entry) {
    roc_panic_if(!valid_);

    if (stop_) {
        return;
    }

    {
        Mutex::Lock lock(mutex_);

        ringbuf_.push_back(entry);
    }

    sem_.post();
}

void CsvDumper::stop() {
    stop_ = true;
    sem_.post();
}

void CsvDumper::run() {
    roc_panic_if(!valid_);

    roc_log(LogDebug, "csv dumper: running background thread");

    while (!stop_ || !ringbuf_.is_empty()) {
        if (ringbuf_.is_empty()) {
            sem_.wait();
        }

        CsvEntry entry;
        while (ringbuf_.pop_front(entry)) {
            if (!allow_(entry)) {
                continue;
            }
            if (!write_(entry)) {
                break;
            }
        }
    }

    roc_log(LogDebug, "csv dumper: exiting background thread");

    close_();
}

bool CsvDumper::allow_(const CsvEntry& entry) {
    roc_panic_if(!isalnum(entry.type));

    const size_t idx = (size_t)entry.type;

    if (!rate_lims_[idx]) {
        rate_lims_[idx].reset(new (rate_lims_[idx]) RateLimiter(config_.max_interval));
    }

    return rate_lims_[idx]->allow();
}

bool CsvDumper::open_(const char* path) {
    roc_panic_if(file_);

    file_ = fopen(path, "w");
    if (!file_) {
        roc_log(LogError, "csv dumper: failed to open output file \"%s\": %s", path,
                errno_to_str().c_str());
        return false;
    }

    return true;
}

void CsvDumper::close_() {
    if (file_) {
        if (fclose(file_) != 0) {
            roc_log(LogError, "csv dumper: failed to close output file: %s",
                    errno_to_str().c_str());
        }
        file_ = NULL;
    }
}

bool CsvDumper::write_(const CsvEntry& entry) {
    enum { MaxLineLen = 256 };

    roc_panic_if(!file_);

    char line[MaxLineLen] = {};
    size_t off = 0;

    int ret = snprintf(line + off, sizeof(line) - off - 1, "%c", entry.type);
    if (ret <= 0) {
        return false;
    }
    off += (size_t)ret;

    for (size_t n = 0; n < entry.n_fields; n++) {
        int ret = snprintf(line + off, sizeof(line) - off - 1, ",%f", entry.fields[n]);
        if (ret <= 0) {
            return false;
        }
        off += (size_t)ret;
    }

    if (fprintf(file_, "%s\n", line) < 0) {
        roc_log(LogError, "csv dumper: failed to write output file: %s",
                errno_to_str().c_str());
        return false;
    }

    return true;
}

} // namespace core
} // namespace roc
