/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cstdio>

#include "roc_core/panic.h"
#include "roc_core/print_buffer.h"

namespace roc {
namespace core {

namespace {

void print_value(uint8_t v) {
    printf(" %02x", (unsigned)v);
}

void print_value(float v) {
    printf(" %.4f", (double)v);
}

template <class T>
void print_data(const char* type, const T* data, size_t size, size_t max_size) {
    enum { MaxPerLine = 10 };

    if (data == NULL) {
        printf("buffer(%s): <null>", type);
    } else {
        printf("buffer(%s): size=%lu, max_size=%lu", type, (unsigned long)size,
               (unsigned long)max_size);

        for (size_t n = 0; n < size; n++) {
            if (n % MaxPerLine == 0) {
                printf("\n ");
            }
            print_value(data[n]);
        }
    }

    printf("\n");
}

} // namespace

void print_buffer(const uint8_t* data, size_t size, size_t max_size) {
    print_data("byte", data, size, max_size);
}

void print_buffer(const float* data, size_t size, size_t max_size) {
    print_data("float", data, size, max_size);
}

} // namespace core
} // namespace roc
