/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_pc/roc_core/log_backend.h
//! @brief Log backend.

#ifndef ROC_CORE_LOG_BACKEND_H_
#define ROC_CORE_LOG_BACKEND_H_

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

struct LogMessage;

//! Log backend.
class LogBackend : public NonCopyable<> {
public:
    LogBackend();

    //! Handle log message.
    void handle(const LogMessage& msg);

private:
    bool colors_supported_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOG_BACKEND_H_
