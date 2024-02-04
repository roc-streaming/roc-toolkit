/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/parse_units.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

namespace {

const char* find_suffix(const char* str, size_t str_len, const char* suffix) {
    const size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) {
        return NULL;
    }
    if (strcmp(str + str_len - suffix_len, suffix) != 0) {
        return NULL;
    }
    return str + str_len - suffix_len;
}

} // namespace

bool parse_duration(const char* str, nanoseconds_t& result) {
    if (str == NULL) {
        roc_log(LogError, "parse duration: string is null");
        return false;
    }

    nanoseconds_t multiplier = 0;

    const size_t str_len = strlen(str);
    const char* suffix;

    if ((suffix = find_suffix(str, str_len, "ns"))) {
        multiplier = Nanosecond;
    } else if ((suffix = find_suffix(str, str_len, "us"))) {
        multiplier = Microsecond;
    } else if ((suffix = find_suffix(str, str_len, "ms"))) {
        multiplier = Millisecond;
    } else if ((suffix = find_suffix(str, str_len, "s"))) {
        multiplier = Second;
    } else if ((suffix = find_suffix(str, str_len, "m"))) {
        multiplier = Minute;
    } else if ((suffix = find_suffix(str, str_len, "h"))) {
        multiplier = Hour;
    } else {
        roc_log(LogError,
                "parse duration: invalid format: missing suffix, expected"
                " <float><suffix>, where suffix=<ns|us|ms|s|m|h>");
        return false;
        return false;
    }

    if (str == suffix) {
        roc_log(LogError,
                "parse duration: invalid format: missing number, expected"
                " <float><suffix>, where suffix=<ns|us|ms|s|m|h>");
        return false;
    }

    if (!isdigit(*str) && *str != '+' && *str != '-') {
        roc_log(LogError,
                "parse duration: invalid format: not a number, expected"
                " <float><suffix>, where suffix=<ns|us|ms|s|m|h>");
        return false;
    }

    char* number_end = NULL;
    const double number = strtod(str, &number_end);

    if ((number == 0. && str == number_end) || !number_end || number_end != suffix) {
        roc_log(LogError,
                "parse duration: invalid format: not a number, expected"
                " <float><suffix>, where suffix=<ns|us|ms|s|m|h>");
        return false;
    }

    const double number_multiplied = round(number * (double)multiplier);

    if (number_multiplied > (double)INT64_MAX || number_multiplied < (double)INT64_MIN) {
        roc_log(LogError,
                "parse duration: number out of range:"
                " value=%f minimim=%f maximum=%f",
                number_multiplied, (double)INT64_MIN, (double)INT64_MAX);
        return false;
    }

    result = (nanoseconds_t)number_multiplied;
    return true;
}

bool parse_size(const char* str, size_t& result) {
    if (str == NULL) {
        roc_log(LogError, "parse size: string is null");
        return false;
    }

    const size_t kibibyte = 1024;
    const size_t mebibyte = 1024 * kibibyte;
    const size_t gibibyte = 1024 * mebibyte;

    size_t multiplier = 1;

    const size_t str_len = strlen(str);
    const char* suffix;

    // suffix is optional.
    if ((suffix = find_suffix(str, str_len, "G"))) {
        multiplier = gibibyte;
    } else if ((suffix = find_suffix(str, str_len, "M"))) {
        multiplier = mebibyte;
    } else if ((suffix = find_suffix(str, str_len, "K"))) {
        multiplier = kibibyte;
    }

    if (!isdigit(*str)) {
        roc_log(LogError,
                "parse size: invalid format: not a number, expected"
                " <float>[<suffix>], where suffix=<K|M|G>");
        return false;
    }

    char* number_end = NULL;
    const double number = strtod(str, &number_end);

    if ((number == 0. && str == number_end) || (!suffix && *number_end != '\0')
        || (suffix && number_end != suffix)) {
        roc_log(LogError,
                "parse size: invalid format: not a number, expected"
                " <float>[<suffix>], where suffix=<K|M|G>");
        return false;
    }

    const double number_multiplied = round(number * (double)multiplier);
    if (number_multiplied > (double)SIZE_MAX) {
        roc_log(LogError,
                "parse size: number out of range:"
                " value=%f maximum=%f",
                number_multiplied, (double)SIZE_MAX);
        return false;
    }

    result = (size_t)number_multiplied;
    return true;
}

} // namespace core
} // namespace roc
