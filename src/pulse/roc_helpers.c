/*
 * This file is part of Roc PulseAudio integration.
 *
 * Copyright (c) 2017 Roc authors
 *
 * Licensed under GNU Lesser General Public License 2.1 or any later version.
 */

/* system headers */
#include <stdlib.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* local headers */
#include "roc_helpers.h"

void log_handler(roc_log_level level, const char* module, const char* message) {
    switch (level) {
    case ROC_LOG_NONE:
        return;

    case ROC_LOG_ERROR:
        pa_log_level_meta(PA_LOG_ERROR, module, -1, NULL, "%s", message);
        return;

    case ROC_LOG_INFO:
        pa_log_level_meta(PA_LOG_INFO, module, -1, NULL, "%s", message);
        return;

    case ROC_LOG_DEBUG:
    case ROC_LOG_TRACE:
        pa_log_level_meta(PA_LOG_DEBUG, module, -1, NULL, "%s", message);
        return;
    }
}

int parse_address(roc_address* addr,
                  pa_modargs* args,
                  const char* ip_arg,
                  const char* default_ip_arg,
                  const char* port_arg,
                  const char* default_port_arg) {
    const char* ip_str;
    if (ip_arg) {
        ip_str = pa_modargs_get_value(args, ip_arg, default_ip_arg);
    } else {
        ip_str = default_ip_arg;
    }

    if (!*ip_str) {
        ip_str = "0.0.0.0";
    }

    const char* port_str;
    if (port_arg) {
        port_str = pa_modargs_get_value(args, port_arg, default_port_arg);
    } else {
        port_str = default_port_arg;
    }

    char* end = NULL;
    long port_num = strtol(port_str, &end, 10);
    if (port_num == LONG_MIN || port_num == LONG_MAX || !end || *end) {
        pa_log("invalid %s: %s", port_arg, port_str);
        return -1;
    }

    if (roc_address_init(addr, ROC_AF_AUTO, ip_str, (int)port_num) != 0) {
        pa_log("invalid address: %s:%s", ip_str, port_str);
        return -1;
    }

    return 0;
}
