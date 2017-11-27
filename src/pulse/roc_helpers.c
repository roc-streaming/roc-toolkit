/* This file is part of Roc PulseAudio integration.
 *
 * Copyright 2017 Victor Gaydov
 * Copyright 2017 Mikhail Baranov
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

int parse_address(struct sockaddr_in* addr,
                  pa_modargs* args,
                  const char* ip_arg,
                  const char* default_ip_arg,
                  const char* port_arg,
                  const char* default_port_arg) {
    const char* port_str;
    if (port_arg) {
        port_str = pa_modargs_get_value(args, port_arg, default_port_arg);
    } else {
        port_str = default_port_arg;
    }

    char* end = NULL;
    long port_num = strtol(port_str, &end, 10);
    if (port_num < 0 || port_num >= 65536 || !end || *end) {
        pa_log("invalid %s: %s", port_arg, port_str);
        return -1;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons((uint16_t)port_num);

    const char* ip_str;
    if (ip_arg) {
        ip_str = pa_modargs_get_value(args, ip_arg, default_ip_arg);
    } else {
        ip_str = default_ip_arg;
    }

    if (*ip_str) {
        if (inet_pton(AF_INET, ip_str, &addr->sin_addr) <= 0) {
            pa_log("invalid %s: %s", ip_arg, ip_str);
            return -1;
        }
    } else {
        addr->sin_addr.s_addr = INADDR_ANY;
    }

    return 0;
}
