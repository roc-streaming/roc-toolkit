/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/print.h"

namespace roc {
namespace core {

namespace {

void print_value(uint8_t v) {
    fprintf(stderr, " %02x", (unsigned)v);
}

void print_value(float v) {
    fprintf(stderr, " %.4f", (double)v);
}

template <class T> void print_buffer(const T* data, size_t size) {
    enum { MaxPerLine = 10 };

    for (size_t n = 0; n < size; n++) {
        if (n % MaxPerLine == 0) {
            fprintf(stderr, "\n ");
        }
        print_value(data[n]);
    }

    fprintf(stderr, "\n");
}

} // namespace

void print_bytes(const uint8_t* data, size_t size) {
    print_buffer(data, size);
}

void print_floats(const float* data, size_t size) {
    print_buffer(data, size);
}

} // namespace core
} // namespace roc
