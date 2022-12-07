/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_stdio/roc_core/printer.h
//! @brief Console printer.

#ifndef ROC_CORE_PRINTER_H_
#define ROC_CORE_PRINTER_H_

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Printer.
class Printer : public NonCopyable<> {
public:
    enum {
        BufferSize = 2048,    //!< Maximum buffer size.
        FlushThreshold = 1536 //!< Threshold after which buffer is flushed.
    };

    //! Printing function.
    typedef void (*PrintFunc)(const char* buf, size_t bufsz);

    //! Initialize printer.
    //! If @p print_func is NULL, prints text to console.
    Printer(PrintFunc print_func = NULL);

    //! Flush and destroy.
    ~Printer();

    //! Write text.
    //! @returns size of formatted string (excluding terminating zero byte).
    ROC_ATTR_PRINTF(2, 3) size_t writef(const char* format, ...);

    //! Flush buffered text to console.
    void flush();

private:
    void flush_(bool force);

    PrintFunc print_;

    char buf_[BufferSize + 1];
    size_t bufsz_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINTER_H_
