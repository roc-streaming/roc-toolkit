/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LIBUNWIND_HH
#define LIBUNWIND_HH
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif
#include <cxxabi.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "roc_core/backtrace.h"

int backtrace()
{
	/* to point to the current frame in the call stack
	 */
	unw_cursor_t cursor;
	/* to store the snapshot of the CPU registers
	 */
	unw_context_t context;

	/* to point to instruction pointer & stack pointer
	 * respectively
	 */
	unw_word_t ip, sp;

	/* to get snapshot of the CPU register
	 */
	unw_getcontext(&context);

	/* to point to current frame 
	 */
	unw_init_local(&cursor, &context);
	/* moving to previously called frames & going
	 * through each of them
	 */
	int size = 0;
	while (unw_step(&cursor) > 0) {
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_IP, &sp);
		size++;
		// fprintf(stderr, "ip = %x\n", (u_int)ip);
	}
	return size;
}

/* print function name, stack frame address and offset
 */
void backtrace_symbols()
{
	/* to point to the current frame in the call stack
	 */
	unw_cursor_t cursor;
	/* to store the snapshot of the CPU registers
	 */
	unw_context_t context;

	/* to get snapshot of the CPU register
	 */
	unw_getcontext(&context);

	/* to point to current frame 
	 */
	unw_init_local(&cursor, &context);
	/* moving to previously called frames & going
	 * through each of them
	 */

	int i = 0;
	while (unw_step(&cursor) > 0) {
		char b[64];
		unw_word_t offset, ip;
		b[0] = '\0';
		/* get value stored in instruction register
		 */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		/* get function name, offset
		 */
		(void) unw_get_proc_name(&cursor, b, sizeof(b), &offset);
		// printing index, function name, cursor, offset, ip
		fprintf(stderr, " #%d : %s %p 0x%x 0x%x\n", ++i, b, cursor, (u_int)offset, (u_int)ip);
	}
}

void backtrace_symbols_fd(int f){
	FILE *fd = fdopen(f, "r+");
	/* to point to the current frame in the call stack
	 */
	unw_cursor_t cursor;
	/* to store the snapshot of the CPU registers
	 */
	unw_context_t context;

	/* to get snapshot of the CPU register
	 */
	unw_getcontext(&context);

	/* to point to current frame 
	 */
	unw_init_local(&cursor, &context);
	/* moving to previously called frames & going
	 * through each of them
	 */

	int i = 0;
	while (unw_step(&cursor) > 0) {
		char b[64];
		unw_word_t offset, ip;
		b[0] = '\0';
		/* get value stored in instruction register
		 */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		/* get function name, offset
		 */
		(void) unw_get_proc_name(&cursor, b, sizeof(b), &offset);
		// printing index, function name, cursor, offset, ip
		fprintf(fd, " #%d : %s %p 0x%x 0x%x\n", ++i,  b, cursor, (u_int)offset, (u_int)ip);
	}
}

namespace roc {
namespace core {

void print_backtrace() {
	int size = backtrace();
	if(size<=0) {
		fprintf(stderr, "No backtrace available\n");
	} else {
		fprintf(stderr, "Backtrace:\n");
		backtrace_symbols();
	}
}

void print_backtrace_emergency() {
	int size = backtrace();
	if(size > 0) {
		backtrace_symbols_fd(STDERR_FILENO);
	}
}

} // namespace core
} // namespace roc
