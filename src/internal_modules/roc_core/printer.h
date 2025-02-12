/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/printer.h
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
        BufferSize = 1024, //!< Maximum buffer size.
    };

    //! Printing function.
    typedef void (*PrintlnFunc)(const char* buf, size_t bufsz);

    //! Initialize printer.
    //! If @p println_func is NULL, prints text to console.
    Printer(PrintlnFunc println_func = NULL);

    //! Flush and destroy.
    ~Printer();

    //! Write text.
    //! @returns size of formatted string (excluding terminating zero byte).
    ROC_PRINTF(2, 3) size_t writef(const char* format, ...);

private:
    void flush_(bool force);

    PrintlnFunc println_;

    char buf_[BufferSize + 1];
    size_t bufsz_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_PRINTER_H_
