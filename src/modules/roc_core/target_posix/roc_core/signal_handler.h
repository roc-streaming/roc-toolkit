/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/signal_handler.h
//! @brief Signal handler.

#ifndef ROC_CORE_SIGNAL_HANDLER_H_
#define ROC_CORE_SIGNAL_HANDLER_H_

#include <signal.h>

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Signal handler.
class SignalHandler : public core::NonCopyable<> {
public:
    //! Install signal handlers.
    SignalHandler();

    //! Restore signal handlers.
    ~SignalHandler();

private:
    void crash_handler_(int sig);

    enum { MaxSigs = 8 };

    struct sigaction sa_restore_[MaxSigs];
    int sig_restore_[MaxSigs];
    size_t restore_sz_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SIGNAL_HANDLER_H_
