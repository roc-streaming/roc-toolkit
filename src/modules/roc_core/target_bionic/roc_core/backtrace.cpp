/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cxxabi.h>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <unwind.h>


#include "roc_core/backtrace.h"

/* Implementation of backtrace in bionic/android is similar
 * to the implementation of backtrace in glibc.
 */


struct BacktraceState
{
    void** current;
    void** end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    _Unwind_Ptr pc = _Unwind_GetIP(context);
    if (pc) {
	   if (state->current == state->end) {
            return _URC_END_OF_STACK;
       } else {
            *state->current++ = reinterpret_cast<void*>(pc);
       }
    }
    return _URC_NO_REASON;
}

size_t captureBacktrace(void** buffer, size_t max)
{
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);

    return state.current - buffer;
}

void dumpBacktrace(void** buffer, size_t count)
{
	if(count <= 0) {
		fprintf(stderr, "No backtrace available\n");
	} else {
		fprintf(stderr, "Backtrace:\n");
		for (size_t idx = 0; idx < count; ++idx) {
			const void* addr = buffer[idx];
			const char* symbol = "";
			int status = -1;

			Dl_info info;
			if (dladdr(addr, &info) && info.dli_sname) {
				symbol = info.dli_sname;
			}
			fprintf(stderr, "#%zd: 0x%x %s", idx, addr, symbol);
			/* perform demangling
			 */
			symbol = abi::__cxa_demangle(symbol, 0, 0, &status);
			if(status == 0)
				fprintf(stderr, " %s", symbol);
			fprintf(stderr, "\n");
		}
	}
}

void dumpBacktrace_fd(void** buffer, size_t count, int f)
{
	FILE *fd = fdopen(f, "r+");
	if(count <= 0) {
		fprintf(fd, "No backtrace available\n");
	} else {
		fprintf(fd, "Backtrace:\n");
		for (size_t idx = 0; idx < count; ++idx) {
			const void* addr = buffer[idx];
			const char* symbol = "";
			int status = -1;

			Dl_info info;
			if (dladdr(addr, &info) && info.dli_sname) {
				symbol = info.dli_sname;
			}
			fprintf(fd, "#%zd: 0x%x %s", idx, addr, symbol);
			/* perform demangling
			 */
			symbol = abi::__cxa_demangle(symbol, 0, 0, &status);
			if(status == 0)
				fprintf(fd, " %s", symbol);
			fprintf(fd, "\n");
		}
	}
}

namespace roc {
namespace core {

namespace {

enum { MaxDepth = 128, MaxLen = 128 };

}

void print_backtrace() 
{
	void* buffer[MaxLen];
	dumpBacktrace(buffer, captureBacktrace(buffer, MaxLen));
}

void print_backtrace_emergency() 
{
	void* buffer[MaxLen];
	dumpBacktrace_fd(buffer, captureBacktrace(buffer, MaxLen), STDERR_FILENO);
}

} // namespace core
} // namespace roc
