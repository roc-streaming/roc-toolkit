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
#include "module_helpers.h"

void rocpa_log_handler(roc_log_level level, const char* module, const char* message) {
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

int rocpa_parse_endpoint(roc_endpoint** endp,
                         roc_protocol proto,
                         pa_modargs* args,
                         const char* ip_arg,
                         const char* default_ip_arg,
                         const char* port_arg,
                         const char* default_port_arg) {
    if (roc_endpoint_allocate(endp) != 0) {
        pa_log("can't allocate endpoint");
        return -1;
    }

    if (roc_endpoint_set_protocol(*endp, proto) != 0) {
        pa_log("can't set endpoint protocol");
        return -1;
    }

    const char* ip_str;
    if (ip_arg) {
        ip_str = pa_modargs_get_value(args, ip_arg, default_ip_arg);
    } else {
        ip_str = default_ip_arg;
    }

    if (!*ip_str) {
        ip_str = "0.0.0.0";
    }

    if (roc_endpoint_set_host(*endp, ip_str) != 0) {
        pa_log("can't set endpoint host");
        return -1;
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

    if (roc_endpoint_set_port(*endp, (int)port_num) != 0) {
        pa_log("can't set endpoint port");
        return -1;
    }

    return 0;
}

int rocpa_parse_duration_msec(unsigned long long* out,
                              unsigned long out_base,
                              pa_modargs* args,
                              const char* arg_name,
                              const char* arg_default) {
    const char* str = pa_modargs_get_value(args, arg_name, arg_default);

    char* end = NULL;
    long num = strtol(str, &end, 10);
    if (num == LONG_MIN || num == LONG_MAX || !end || *end) {
        pa_log("invalid %s: not a number: %s", arg_name, str);
        return -1;
    }

    if (num < 0) {
        pa_log("invalid %s: should not be negative: %s", arg_name, str);
        return -1;
    }

    *out = (unsigned long long)num * (1000000 / out_base);
    return 0;
}

int rocpa_parse_resampler_profile(roc_resampler_profile* out,
                                  pa_modargs* args,
                                  const char* arg_name) {
    const char* str = pa_modargs_get_value(args, arg_name, "");

    if (!str || !*str) {
        *out = ROC_RESAMPLER_PROFILE_DEFAULT;
        return 0;
    } else if (strcmp(str, "disable") == 0) {
        *out = ROC_RESAMPLER_PROFILE_DISABLE;
        return 0;
    } else if (strcmp(str, "high") == 0) {
        *out = ROC_RESAMPLER_PROFILE_HIGH;
        return 0;
    } else if (strcmp(str, "medium") == 0) {
        *out = ROC_RESAMPLER_PROFILE_MEDIUM;
        return 0;
    } else if (strcmp(str, "low") == 0) {
        *out = ROC_RESAMPLER_PROFILE_LOW;
        return 0;
    } else {
        pa_log("invalid %s: %s", arg_name, str);
        return -1;
    }
}
