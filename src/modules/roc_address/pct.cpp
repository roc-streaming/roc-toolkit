/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/pct.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

namespace {

// see RFC 3986

bool is_unreserved(char c) {
    if (isalnum(c)) {
        return true;
    }
    switch (c) {
    case '-':
    case '_':
    case '.':
    case '~':
        return true;
    default:
        break;
    }
    return false;
}

bool is_subdelim(char c) {
    switch (c) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
        return true;
    default:
        break;
    }
    return false;
}

bool is_pchar(char c) {
    if (is_unreserved(c)) {
        return true;
    }
    if (is_subdelim(c)) {
        return true;
    }
    switch (c) {
    case ':':
    case '@':
        return true;
    default:
        break;
    }
    return false;
}

bool is_segment_char(char c) {
    return is_pchar(c) || c == '/';
}

bool is_ip_literal_char(char c) {
    if (isxdigit(c)) {
        return true;
    }
    switch (c) {
    case '.':
    case ':':
    case '[':
    case ']':
        return true;
    default:
        break;
    }
    return false;
}

bool is_regname_char(char c) {
    if (is_unreserved(c)) {
        return true;
    }
    if (is_subdelim(c)) {
        return true;
    }
    return false;
}

bool is_host_char(char c) {
    return is_ip_literal_char(c) || is_regname_char(c);
}

char to_hex(unsigned char c) {
    return "0123456789ABCDEF"[c & 0xf];
}

char from_hex(char hi, char lo) {
    const int h = isdigit(hi) ? hi - '0' : tolower(hi) - 'a' + 10;
    const int l = isdigit(lo) ? lo - '0' : tolower(lo) - 'a' + 10;
    return char(h << 4 | l);
}

} // namespace

ssize_t
pct_encode(char* dst, size_t dst_sz, const char* src, size_t src_sz, PctMode mode) {
    roc_panic_if(dst == NULL);
    roc_panic_if(src == NULL);

    const char* dst_start = dst;
    const char* dst_end = dst + dst_sz;

    const char* src_end = src + src_sz;

    bool (*skip_encoding)(char) = NULL;
    switch (mode) {
    case PctNonUnreserved:
        skip_encoding = is_unreserved;
        break;
    case PctNonHost:
        skip_encoding = is_host_char;
        break;
    case PctNonPath:
        skip_encoding = is_segment_char;
        break;
    }

    while (src < src_end) {
        if (*src == '\0') {
            return -1;
        }

        if (skip_encoding(*src)) {
            if (dst_end - dst < 1) {
                return -1;
            }
            *dst++ = *src++;
            continue;
        }

        if (dst_end - dst < 3) {
            return -1;
        }
        *dst++ = '%';
        *dst++ = to_hex((unsigned char)*src >> 4);
        *dst++ = to_hex((unsigned char)*src & 0xf);
        src++;
    }

    if (dst_end - dst < 1) {
        return -1;
    }
    *dst = '\0';

    return ssize_t(dst - dst_start);
}

ssize_t pct_decode(char* dst, size_t dst_sz, const char* src, size_t src_sz) {
    roc_panic_if(dst == NULL);
    roc_panic_if(src == NULL);

    const char* dst_start = dst;
    const char* dst_end = dst + dst_sz;

    const char* src_end = src + src_sz;

    while (src < src_end) {
        if (*src == '\0') {
            return -1;
        }

        if (*src == '%') {
            if (dst_end - dst < 1) {
                return -1;
            }
            if (src_end - src < 3) {
                return -1;
            }
            if (!isxdigit(src[1]) || !isxdigit(src[2])) {
                return -1;
            }
            const char c = from_hex(src[1], src[2]);
            if (c == '\0') {
                return -1;
            }
            *dst++ = c;
            src += 3;
            continue;
        }

        if (dst_end - dst < 1) {
            return -1;
        }
        *dst++ = *src++;
    }

    if (dst_end - dst < 1) {
        return -1;
    }
    *dst = '\0';

    return ssize_t(dst - dst_start);
}

} // namespace address
} // namespace roc
