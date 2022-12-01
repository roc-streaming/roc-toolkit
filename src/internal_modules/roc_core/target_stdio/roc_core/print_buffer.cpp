/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/print_buffer.h"

#include <stdio.h>

namespace roc {
namespace core {

namespace {

template <class T> const char* type_name();

template <> const char* type_name<uint8_t>() {
    return "uint8";
}

template <> const char* type_name<uint16_t>() {
    return "uint16";
}

template <> const char* type_name<uint32_t>() {
    return "uint32";
}

template <> const char* type_name<uint64_t>() {
    return "uint64";
}

template <> const char* type_name<int8_t>() {
    return "int8";
}

template <> const char* type_name<int16_t>() {
    return "int16";
}

template <> const char* type_name<int32_t>() {
    return "int32";
}

template <> const char* type_name<int64_t>() {
    return "int64";
}

template <> const char* type_name<float>() {
    return "float";
}

template <> const char* type_name<double>() {
    return "double";
}

void print_number_t(uint8_t v) {
    fprintf(stderr, "%02x", (unsigned)v);
}

void print_number_t(uint16_t v) {
    fprintf(stderr, "%6u", (unsigned)v);
}

void print_number_t(uint32_t v) {
    fprintf(stderr, "%11lu", (unsigned long)v);
}

void print_number_t(uint64_t v) {
    fprintf(stderr, "%21llu", (unsigned long long)v);
}

void print_number_t(int8_t v) {
    fprintf(stderr, "%4d", (int)v);
}

void print_number_t(int16_t v) {
    fprintf(stderr, "%6d", (int)v);
}

void print_number_t(int32_t v) {
    fprintf(stderr, "%11ld", (long)v);
}

void print_number_t(int64_t v) {
    fprintf(stderr, "%21lld", (long long)v);
}

void print_number_t(float v) {
    fprintf(stderr, "%.4f", (double)v);
}

void print_number_t(double v) {
    fprintf(stderr, "%.6f", v);
}

template <class T> void print_impl_t(const T* data, size_t size, size_t from, size_t to) {
    if (size == 0) {
        return;
    }

    size_t max_per_line = 1;

    switch (sizeof(T)) {
    case 1:
        max_per_line = 16;
        break;

    case 2:
        max_per_line = 10;
        break;

    case 4:
        max_per_line = 5;
        break;

    case 8:
        max_per_line = 3;
        break;
    }

    for (size_t n = 0;; n++) {
        const bool nl = (n != 0) && (n % max_per_line == 0);

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

template <class T> void print_buffer_t(const T* data, size_t size) {
    fprintf(stderr, "@ buffer: type=%s size=%lu\n", type_name<T>(), (unsigned long)size);

    print_impl_t(data, size, (size_t)-1, (size_t)-1);
}

template <class T>
void print_buffer_slice_t(const T* inner,
                          size_t inner_size,
                          const T* outer,
                          size_t outer_size) {
    const size_t off = size_t(inner - outer);

    fprintf(stderr, "@ slice: off=%lu size=%lu cap=%lu\n", (unsigned long)off,
            (unsigned long)inner_size, (unsigned long)outer_size - off);

    if (outer) {
        print_impl_t(outer, outer_size, off, off + inner_size);
    }
}

} // namespace

void print_buffer(const uint8_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const uint16_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const uint32_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const uint64_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const int8_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const int16_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const int32_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const int64_t* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const float* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer(const double* data, size_t size) {
    print_buffer_t(data, size);
}

void print_buffer_slice(const uint8_t* inner,
                        size_t inner_size,
                        const uint8_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const uint16_t* inner,
                        size_t inner_size,
                        const uint16_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const uint32_t* inner,
                        size_t inner_size,
                        const uint32_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const uint64_t* inner,
                        size_t inner_size,
                        const uint64_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const int8_t* inner,
                        size_t inner_size,
                        const int8_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const int16_t* inner,
                        size_t inner_size,
                        const int16_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const int32_t* inner,
                        size_t inner_size,
                        const int32_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const int64_t* inner,
                        size_t inner_size,
                        const int64_t* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const float* inner,
                        size_t inner_size,
                        const float* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

void print_buffer_slice(const double* inner,
                        size_t inner_size,
                        const double* outer,
                        size_t outer_size) {
    print_buffer_slice_t(inner, inner_size, outer, outer_size);
}

} // namespace core
} // namespace roc
