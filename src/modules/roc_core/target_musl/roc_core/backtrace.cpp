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
#include <string.h>
#include <unistd.h>

#include "roc_core/backtrace.h"

/* Function to convert unsigned int to decimal/hex string.
 * This function is signal safe.
 * Parameters : 
 * number = the number to be converted.
 * base = 10 for decimal to char* and 16 for hex to char* conversion.
 * output_string = the decimal/hex string
 */
void uint_to_string(unsigned int number, int base, char *output_string)
{
	int i = 12;
	int j = 0;
	do {
		output_string[i] = "0123456789abcdef"[number % base];
		i--;
		number = number/base;
	} while(number > 0);

	while(++i < 13)
		output_string[j++] = output_string[i];
	output_string[j] = 0;
}

/* Return size of backtrace stack by unwinding it.
 * This must be signal safe.
 */
ssize_t backtrace()
{
	/* To point to the current frame in the call stack.
	 */
	unw_cursor_t cursor;

	/* To store the snapshot of the CPU registers.
	 */
	unw_context_t context;

	/* To point to instruction pointer & stack pointer respectively.
	 */
	unw_word_t ip, sp;

	/* To get snapshot of the CPU register.
	 * unw_getcontext() is signal safe.
	 */
	if(unw_getcontext(&context) < 0)
		return -1;

	/* To point to current frame.
	 * unw_init_local() is signal safe.
	 */
	if(unw_init_local(&cursor, &context) < 0)
		return -1;

	/* Moving to previously called frames & going through each of them.
	 * unw_step() is signal safe.
	 */
	ssize_t size = 0;
	while (unw_step(&cursor) > 0)
		size++;
	return size;
}

/* Print function name, stack frame address and offset.
 * Not signal safe as we are using fprintf() here.
 */
void backtrace_symbols()
{
	/* To point to the current frame in the call stack.
	 */
	unw_cursor_t cursor;

	/* To store the snapshot of the CPU registers.
	 */
	unw_context_t context;

	/* To get snapshot of the CPU register.
	 * unw_getcontext() is signal safe.
	 */
	unw_getcontext(&context);

	/* To point to current frame. 
	 */
	unw_init_local(&cursor, &context);

	/* Moving to previously called frames & going through each of them.
	 */
	int i = 0;
	while (unw_step(&cursor) > 0) {
		char b[128];
		unw_word_t offset, ip;
		b[0] = '\0';
		/* Get value stored in instruction register.
		 * unw_get_reg() is signal safe.
		 */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);

		/* Get function name, offset.
		 * unw_get_proc_name() is signal safe.
		 */
		(void) unw_get_proc_name(&cursor, b, sizeof(b), &offset);
		
		/* Perform demangling
		 */
		char *demangled;
		int status = -1;
		if(offset > 0) {
			char mangled[128];
			memcpy(mangled, b, 128);
			demangled = abi::__cxa_demangle(mangled, 0, 0, &status);
		}
		/* printing index, function name, cursor, offset, ip
		 */
		fprintf(stderr, "#%d : (%s+0x%x) [0x%x]\n", ++i, (status == 0)? demangled : b, (u_int)offset, (u_int)ip);
	}
}

/* This must be signal safe.
 */
void backtrace_symbols_fd(int fd){
	/* To point to the current frame in the call stack.
	 */
	unw_cursor_t cursor;

	/* To store the snapshot of the CPU registers.
	 */
	unw_context_t context;

	/* To get snapshot of the CPU register.
	 * unw_getcontext() is signal safe.
	 */
	unw_getcontext(&context);

	/* To point to current frame.
	 * unw_init_local() is signal safe. 
	 */
	unw_init_local(&cursor, &context);

	/* Moving to previously called frames & going through each of them.
	 * unw_step() is signal safe.
	 * buffer = final output that will be written to stderr. 
	 */
	u_int index = 0;
	while (unw_step(&cursor) > 0) {
		char buffer[200] = "#";
		index++;
		char function_name[128];
		unw_word_t offset, ip;
		function_name[0] = '\0';
		/* Get value stored in instruction register
		 * unw_get_reg() is signal safe.
		 */
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		/* Get function name & offset
		 * unw_get_proc_name() is signal safe.
		 */
		(void) unw_get_proc_name(&cursor, function_name, 40, &offset);
		/* Printing index, function name, cursor, offset, ip
		 * using write(). 
		 * write() & strcat() are signal safe.
		 */
		char index_buffer[10];
		uint_to_string(index, 10, index_buffer);
		strcat(buffer, index_buffer);
		strcat(buffer, ": (");
		strcat(buffer, function_name);
		strcat(buffer, "+0x");
		
		char offset_buffer[10];
		uint_to_string((u_int)offset, 16, offset_buffer);
		strcat(buffer, offset_buffer);
		strcat(buffer, ") [");

		char address_buffer[16];
		uint_to_string((u_int)ip, 16, address_buffer);
		strcat(buffer, address_buffer);
		strcat(buffer, "]\n");

		write(fd, buffer, 200);
		// fprintf(fd, " #%d : %s %p 0x%x 0x%x\n", ++i,  b, cursor, (u_int)offset, (u_int)ip);
	}
}

namespace roc {
namespace core {

void print_backtrace() {
	ssize_t size = backtrace();
	if(size <= 0) {
		fprintf(stderr, "No backtrace available\n");
	} else {
		fprintf(stderr, "Backtrace:\n");
		backtrace_symbols();
	}
}

void print_backtrace_emergency() {
	ssize_t size = backtrace();
	if(size > 0) {
		backtrace_symbols_fd(STDERR_FILENO);
	}
}

} // namespace core
} // namespace roc
