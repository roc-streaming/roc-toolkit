/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/signal_handler.h"
#include "roc_core/attributes.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

namespace {

// thread-safe strsignal()
const char* signame(int sig) {
    switch (sig) {
    case SIGSEGV:
        return "SIGSEGV";
    case SIGILL:
        return "SIGILL";
#ifdef SIGBUS
    case SIGBUS:
        return "SIGBUS";
#endif // SIGBUS
#ifdef SIGSTKFLT
    case SIGSTKFLT:
        return "SIGSTKFLT";
#endif // SIGSTKFLT
    case SIGFPE:
        return "SIGFPE";
    default:
        break;
    }
    return "unknown signal";
}

ROC_ATTR_NORETURN void handle_crash(int sig) {
    roc_panic("caught %s", signame(sig));
}

} // namespace

SignalHandler::SignalHandler()
    : restore_sz_(0) {
    crash_handler_(SIGSEGV);
    crash_handler_(SIGILL);
#ifdef SIGBUS
    crash_handler_(SIGBUS);
#endif // SIGBUS
#ifdef SIGSTKFLT
    crash_handler_(SIGSTKFLT);
#endif // SIGSTKFLT
    crash_handler_(SIGFPE);
}

SignalHandler::~SignalHandler() {
    for (size_t n = 0; n < restore_sz_; n++) {
        if (sigaction(sig_restore_[n], &sa_restore_[n], NULL) != 0) {
            roc_panic("signal handler: sigaction(): %s", errno_to_str().c_str());
        }
    }
}

void SignalHandler::crash_handler_(int sig) {
    roc_panic_if(restore_sz_ == MaxSigs);

    struct sigaction sa = {};
    sa.sa_handler = handle_crash;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(sig, &sa, &sa_restore_[restore_sz_]) != 0) {
        roc_panic("signal handler: sigaction(): %s", errno_to_str().c_str());
    }

    sig_restore_[restore_sz_] = sig;
    restore_sz_++;
}

} // namespace core
} // namespace roc
