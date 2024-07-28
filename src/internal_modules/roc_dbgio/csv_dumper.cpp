/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_dbgio/csv_dumper.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace dbgio {

CsvDumper::CsvDumper(const CsvConfig& config, core::IArena& arena)
    : config_(config)
    , open_flag_(false)
    , stop_flag_(false)
    , file_(NULL)
    , ringbuf_(arena, config.max_queued) {
    if (!config.dump_file) {
        roc_panic("csv dumper: dump file is null");
    }
}

CsvDumper::~CsvDumper() {
    if (open_flag_ && !stop_flag_) {
        roc_panic("csv dumper: close() not called before destructor");
    }
}

status::StatusCode CsvDumper::open() {
    core::Mutex::Lock lock(open_mutex_);

    if (open_flag_) {
        roc_panic("csv dumper: open() already called");
    }

    open_flag_ = true;

    if (!open_(config_.dump_file)) {
        return status::StatusErrFile;
    }

    if (!Thread::start()) {
        return status::StatusErrThread;
    }

    return status::StatusOK;
}

void CsvDumper::close() {
    core::Mutex::Lock lock(open_mutex_);

    stop_flag_ = true;
    write_sem_.post();

    Thread::join();

    close_();
}

bool CsvDumper::would_write(char type) {
    roc_panic_if(!open_flag_);

    if (stop_flag_) {
        return false;
    }

    if (!write_mutex_.try_lock()) {
        return false;
    }

    const bool would = limiter_(type).would_allow();

    write_mutex_.unlock();

    return would;
}

void CsvDumper::write(const CsvEntry& entry) {
    roc_panic_if(!open_flag_);

    if (stop_flag_) {
        return;
    }

    if (!write_mutex_.try_lock()) {
        return;
    }

    if (!limiter_(entry.type).allow()) {
        write_mutex_.unlock();
        return;
    }

    ringbuf_.push_back(entry);

    write_mutex_.unlock();
    write_sem_.post();
}

void CsvDumper::run() {
    roc_log(LogDebug, "csv dumper: running background thread");

    while (!stop_flag_ || !ringbuf_.is_empty()) {
        if (ringbuf_.is_empty()) {
            write_sem_.wait();
        }

        CsvEntry entry;
        while (ringbuf_.pop_front(entry)) {
            if (!dump_(entry)) {
                break;
            }
        }
    }

    roc_log(LogDebug, "csv dumper: exiting background thread");
}

core::RateLimiter& CsvDumper::limiter_(char type) {
    roc_panic_if(!isalnum(type));

    const size_t idx = (size_t)type;

    if (!rate_lims_[idx]) {
        rate_lims_[idx].reset(new (rate_lims_[idx])
                                  core::RateLimiter(config_.max_interval));
    }

    return *rate_lims_[idx];
}

bool CsvDumper::open_(const char* path) {
    roc_panic_if(file_);

    file_ = fopen(path, "w");
    if (!file_) {
        roc_log(LogError, "csv dumper: failed to open output file \"%s\": %s", path,
                core::errno_to_str().c_str());
        return false;
    }

    return true;
}

void CsvDumper::close_() {
    if (file_) {
        if (fclose(file_) != 0) {
            roc_log(LogError, "csv dumper: failed to close output file: %s",
                    core::errno_to_str().c_str());
        }
        file_ = NULL;
    }
}

bool CsvDumper::dump_(const CsvEntry& entry) {
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
                core::errno_to_str().c_str());
        return false;
    }

    return true;
}

} // namespace dbgio
} // namespace roc
