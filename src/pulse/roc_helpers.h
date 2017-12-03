/* This file is part of Roc PulseAudio integration.
 *
 * Copyright 2017 Victor Gaydov
 * Copyright 2017 Mikhail Baranov
 *
 * Licensed under GNU Lesser General Public License 2.1 or any later version.
 */

/* config.h from pulseaudio directory (generated after ./configure) */
#include <config.h>

/* private pulseaudio headers */
#include <pulsecore/modargs.h>

/* roc headers */
#include <roc/log.h>

/* system headers */
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_IP "0.0.0.0"
#define DEFAULT_SOURCE_PORT "10001"
#define DEFAULT_REPAIR_PORT "10002"

void log_handler(roc_log_level level, const char* module, const char* message);

int parse_address(struct sockaddr_in* addr,
                  pa_modargs* args,
                  const char* ip_arg,
                  const char* default_ip_arg,
                  const char* port_arg,
                  const char* default_port_arg);
