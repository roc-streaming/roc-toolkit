/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */


#define UNW_LOCAL_ONLY
#include <libunwind.h>

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

enum { MaxDigits = 10 , DefaultLen = 200};


void uint32_to_string(uint32_t number, uint32_t base, char *output_string)
{
	int i = MaxDigits;
	int j = 0;
	do {
		output_string[i] = "0123456789abcdef"[number % base];
		i--;
		number = number/base;
	} while(number > 0);

	while(++i < MaxDigits+1)
		output_string[j++] = output_string[i];
	output_string[j] = 0;
}

/* Checking if backtrace is empty or not.
 * This must be signal safe.
 */
bool is_backtrace_available()
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
	if(unw_getcontext(&context) < 0)
		return false;

	/* To point to current frame.
	 * unw_init_local() is signal safe.
	 */
	if(unw_init_local(&cursor, &context) < 0)
		return false;

	/* Moving to previously called frames & going through each of them.
	 * unw_step() is signal safe.
	 * If there is atleast one entry in backtrace
	 */
	if (unw_step(&cursor) > 0)
		return true;
	return false;
}

/* Print function name, offset, instruction pointer address
 * This must be signal safe.
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
	uint32_t index = 0;
	while (unw_step(&cursor) > 0) {
		char buffer[DefaultLen] = "#";
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
		 * 'status' checks if unw_get_proc_name() is successful or not.
		 * 'status = 0', successfully executed the function.
		 * if status is negative, then we'll stop.
		 * This is optional and might be useful for debugging.
		 */
		int32_t status = unw_get_proc_name(&cursor, function_name, 40, &offset);
		if(status < 0) {
			if(status == UNW_EUNSPEC) {
				write(fd, "An unspecified error occured.", 30);
			}
			else if(status == UNW_ENOINFO) {
				write(fd, "Unable to determine name of procedure.", 39);
			}
			else if(status == UNW_ENOMEM) {
				write(fd, "Procedure was too long to fit in the buffer.", 50);
			}
			else {
				write(fd, "Error not found.", 16);
			}
		}
		else {
			/* Printing => index, function name, offset, ip
			 * using write(). 
			 * write() & strcat() are signal safe.
			 */
			char index_buffer[MaxDigits+1];
			uint32_to_string(index, MaxDigits, index_buffer);
			strcat(buffer, index_buffer);
			strcat(buffer, ": (");
			strcat(buffer, function_name);
			strcat(buffer, "+0x");
		
			char offset_buffer[MaxDigits+1];
			uint32_to_string((uint32_t)offset, MaxDigits, offset_buffer);
			strcat(buffer, offset_buffer);
			strcat(buffer, ") [");

			char address_buffer[MaxDigits+1];
			uint32_to_string((uint32_t)ip, MaxDigits, address_buffer);
			strcat(buffer, address_buffer);
			strcat(buffer, "]\n");

			write(fd, buffer, 200);
		}
	}
}

namespace roc {
namespace core {

void print_backtrace() {
	if(!is_backtrace_available()) {
		fprintf(stderr, "No backtrace available\n");
	} else {
		fprintf(stderr, "Backtrace:\n");
		backtrace_symbols_fd(STDERR_FILENO);
	}
}

void print_backtrace_emergency() {
	if(is_backtrace_available()) {
		backtrace_symbols_fd(STDERR_FILENO);
	}
}

} // namespace core
} // namespace roc

int main(){
	return 0;
}
