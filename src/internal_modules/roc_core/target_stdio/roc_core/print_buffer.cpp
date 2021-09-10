/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/print_buffer.h"

namespace roc {
namespace core {

namespace {

void print_number_t(uint8_t v) {
    fprintf(stderr, "%02x", (unsigned)v);
}

void print_number_t(float v) {
    fprintf(stderr, "%.4f", (double)v);
}

template <class T>
void print_buffer_t(const T* data, size_t size, size_t from, size_t to) {
    enum { MaxPerLine = 16 };

    if (size == 0) {
        return;
    }

    for (size_t n = 0;; n++) {
        const bool nl = (n != 0) && (n % MaxPerLine == 0);

        if (n == from) {
            if (nl) {
                fprintf(stderr, "\n [");
            } else {
                fprintf(stderr, " [");
            }
        } else if (n == to) {
            if (n == size) {
                fprintf(stderr, "]\n");
            } else if (nl) {
                fprintf(stderr, "]\n  ");
            } else {
                fprintf(stderr, "] ");
            }
        } else if (n == size) {
            fprintf(stderr, "\n");
        } else {
            if (nl) {
                fprintf(stderr, "\n  ");
            } else {
                fprintf(stderr, "  ");
            }
        }

        if (n == size) {
            break;
        }

        print_number_t(data[n]);
    }
}

template <class T>
void print_buffer_slice_t(const T* inner,
                          size_t inner_size,
                          const T* outer,
                          size_t outer_size) {
    const size_t off = size_t(inner - outer);

    fprintf(stderr, "slice: off=%lu size=%lu cap=%lu\n", (unsigned long)off,
            (unsigned long)inner_size, (unsigned long)outer_size - off);

    if (outer) {
        print_buffer_t(outer, outer_size, off, off + inner_size);
    }
}

} // namespace

void print_buffer(const uint8_t* data, size_t size) {
    print_buffer_t(data, size, (size_t)-1, (size_t)-1);
}

void print_buffer(const float* data, size_t size) {
    print_buffer_t(data, size, (size_t)-1, (size_t)-1);
}

void print_buffer_slice(const uint8_t* inner,
                        size_t inner_size,
                        const uint8_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const float* inner,
                        size_t inner_size,
                        const float* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

} // namespace core
} // namespace roc
