/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/csv_dumper.h
//! @brief Asynchronous CSV dumper.

#ifndef ROC_CORE_CSV_DUMPER_H_
#define ROC_CORE_CSV_DUMPER_H_

#include "roc_core/atomic.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/semaphore.h"
#include "roc_core/spsc_ring_buffer.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

//! Maximum number of fields in CSV entry.
static const size_t Csv_MaxFields = 10;

//! CSV entry.
//! Corresponds to one line in output file.
struct CsvEntry {
    char type;                    //!< One-character entry type (first field).
    size_t n_fields;              //!< Number of fields.
    double fields[Csv_MaxFields]; //!< Fields.

    CsvEntry()
        : type('\0')
        , n_fields(0) {
    }
};

//! CSV write configuration.
struct CsvConfig {
    //! Path to the output CSV file.
    //! Can't be null.
    const char* dump_file;

    //! Maximum number of queued entries.
    //! If queue becomes larger, entries are dropped.
    size_t max_queued;

    //! Maximum allowed interval between subsequent entries of same type.
    //! If zero, there is no limit.
    //! If non-zero, each entry type is rate-limited according to this.
    nanoseconds_t max_interval;

    CsvConfig()
        : dump_file(NULL)
        , max_queued(1000)
        , max_interval(Millisecond) {
    }
};

//! Asynchronous CSV dumper.
//! Writes entries to CSV file from background thread.
//! Recommended to be used from a single thread.
class CsvDumper : public Thread {
public:
    //! Open file.
    //! @p path - output file.
    //! @p max_interval - maximum number of writes per second for each entry type.
    CsvDumper(const CsvConfig& config, IArena& arena);

    //! Close file.
    ~CsvDumper();

    //! Check if opened without errors.
    bool is_valid() const;

    //! Check whether write() would enqueue or drop entry.
    //! Lock-free operation.
    bool would_write(char type);

    //! Enqueue entry for writing.
    //! Makes a copy of entry and pushes it to a lock-free ring buffer.
    //! If buffer size limit or rate limit is exceeded, entry is dropped.
    //! Lock-free operation.
    void write(const CsvEntry& entry);

    //! Stop background thread.
    void stop();

private:
    virtual void run();

    RateLimiter& limiter_(char type);

    bool open_(const char* path);
    void close_();
    bool dump_(const CsvEntry& entry);

    const CsvConfig config_;

    FILE* file_;

    Mutex write_mutex_;
    Semaphore write_sem_;
    SpscRingBuffer<CsvEntry> ringbuf_;

    Optional<RateLimiter> rate_lims_[128];

    Atomic<int> stop_;
    bool valid_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_CSV_DUMPER_H_
