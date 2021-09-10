/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/crash.h
//! @brief Crash handling.

#ifndef ROC_CORE_CRASH_H_
#define ROC_CORE_CRASH_H_

#include <signal.h>

#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Report a crash and abort application.
ROC_ATTR_NORETURN void crash(const char* message);

//! Crash handler.
class CrashHandler : public core::NonCopyable<> {
public:
    //! Install signal handlers.
    CrashHandler();

    //! Restore signal handlers.
    ~CrashHandler();

private:
    void install_(int sig);

    enum { MaxSigs = 8 };

    struct sigaction sa_restore_[MaxSigs];
    int sig_restore_[MaxSigs];
    size_t restore_sz_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_CRASH_H_
