/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/crash_handler.h"
#include "roc_core/die.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

namespace roc {
namespace core {

namespace {

const char* signal_string(int sig, siginfo_t* si) {
    switch (sig) {
    case SIGABRT:
        return "caught SIGABRT";
    case SIGSEGV:
        return "caught SIGSEGV";
    case SIGILL:
        return "caught SIGILL";
#ifdef SIGBUS
    case SIGBUS:
        return "caught SIGBUS";
#endif // SIGBUS
#ifdef SIGSTKFLT
    case SIGSTKFLT:
        return "caught SIGSTKFLT";
#endif // SIGSTKFLT
    case SIGFPE:
        switch (si->si_code) {
        case FPE_INTDIV:
            return "caught SIGFPE (integer divide by zero)";
        case FPE_INTOVF:
            return "caught SIGFPE (integer overflow)";
        case FPE_FLTDIV:
            return "caught SIGFPE (floating-point divide by zero)";
        case FPE_FLTOVF:
            return "caught SIGFPE (floating-point overflow)";
        case FPE_FLTUND:
            return "caught SIGFPE (floating-point underflow)";
        case FPE_FLTRES:
            return "caught SIGFPE (floating-point inexact result)";
        case FPE_FLTINV:
            return "caught SIGFPE (floating-point invalid operation)";
        case FPE_FLTSUB:
            return "caught SIGFPE (subscript out of range)";
        default:
            break;
        }
        return "caught SIGFPE";
    default:
        break;
    }
    return "caught unknown signal";
}

ROC_ATTR_NORETURN void signal_handler(int sig, siginfo_t* si, void*) {
    die_gracefully(signal_string(sig, si), false);
}

} // namespace

CrashHandler::CrashHandler()
    : restore_sz_(0) {
    install_(SIGABRT);
    install_(SIGSEGV);
    install_(SIGILL);
#ifdef SIGBUS
    install_(SIGBUS);
#endif // SIGBUS
#ifdef SIGSTKFLT
    install_(SIGSTKFLT);
#endif // SIGSTKFLT
    install_(SIGFPE);
}

CrashHandler::~CrashHandler() {
    uninstall_();
}

void CrashHandler::install_(int sig) {
    roc_panic_if(restore_sz_ == MaxSigs);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = int(SA_SIGINFO | SA_RESETHAND);

    if (sigemptyset(&sa.sa_mask) != 0) {
        roc_log(LogError, "crash handler: sigemptyset(): %s", errno_to_str().c_str());
    }

    if (sigaction(sig, &sa, &sa_restore_[restore_sz_]) != 0) {
        roc_log(LogError, "crash handler: sigaction(): %s", errno_to_str().c_str());
    }

    sig_restore_[restore_sz_] = sig;
    restore_sz_++;
}

void CrashHandler::uninstall_() {
    for (size_t n = 0; n < restore_sz_; n++) {
        if (sigaction(sig_restore_[n], &sa_restore_[n], NULL) != 0) {
            roc_log(LogError, "crash handler: sigaction(): %s", errno_to_str().c_str());
        }
    }
}

} // namespace core
} // namespace roc
