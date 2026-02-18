/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/print_memory.h"
#include "roc_core/printer.h"

namespace roc {
namespace core {

namespace {

template <class T> const char* type_to_str();

template <> const char* type_to_str<uint8_t>() {
    return "uint8";
}

template <> const char* type_to_str<uint16_t>() {
    return "uint16";
}

template <> const char* type_to_str<uint32_t>() {
    return "uint32";
}

template <> const char* type_to_str<uint64_t>() {
    return "uint64";
}

template <> const char* type_to_str<int8_t>() {
    return "int8";
}

template <> const char* type_to_str<int16_t>() {
    return "int16";
}

template <> const char* type_to_str<int32_t>() {
    return "int32";
}

template <> const char* type_to_str<int64_t>() {
    return "int64";
}

template <> const char* type_to_str<float>() {
    return "float";
}

template <> const char* type_to_str<double>() {
    return "double";
}

void print_number_t(Printer& p, uint8_t v) {
    p.writef("%02x", (unsigned)v);
}

void print_number_t(Printer& p, uint16_t v) {
    p.writef("%6u", (unsigned)v);
}

void print_number_t(Printer& p, uint32_t v) {
    p.writef("%11lu", (unsigned long)v);
}

void print_number_t(Printer& p, uint64_t v) {
    p.writef("%21llu", (unsigned long long)v);
}

void print_number_t(Printer& p, int8_t v) {
    p.writef("%4d", (int)v);
}

void print_number_t(Printer& p, int16_t v) {
    p.writef("%6d", (int)v);
}

void print_number_t(Printer& p, int32_t v) {
    p.writef("%11ld", (long)v);
}

void print_number_t(Printer& p, int64_t v) {
    p.writef("%21lld", (long long)v);
}

void print_number_t(Printer& p, float v) {
    p.writef("%.6f", (double)v);
}

void print_number_t(Printer& p, double v) {
    p.writef("%.6f", v);
}

template <class T>
void print_impl_t(Printer& p, const T* data, size_t size, size_t from, size_t to) {
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
                p.writef("\n [");
            } else {
                p.writef(" [");
            }
        } else if (n == to) {
            if (n == size) {
                p.writef("]\n");
            } else if (nl) {
                p.writef("]\n  ");
            } else {
                p.writef("] ");
            }
        } else if (n == size) {
            p.writef("\n");
        } else {
            if (nl) {
                p.writef("\n  ");
            } else {
                p.writef("  ");
            }
        }

        if (n == size) {
            break;
        }

        print_number_t(p, data[n]);
    }
}

template <class T> void print_memory_t(const T* data, size_t size) {
    Printer p;

    p.writef("@ buffer: type=%s size=%lu\n", type_to_str<T>(), (unsigned long)size);

    print_impl_t(p, data, size, (size_t)-1, (size_t)-1);
}

template <class T>
void print_memory_slice_t(const T* inner,
                          size_t inner_size,
                          const T* outer,
                          size_t outer_size) {
    Printer p;

    const size_t off = size_t(inner - outer);

    p.writef("@ slice: type=%s off=%lu size=%lu cap=%lu\n", type_to_str<T>(),
             (unsigned long)off, (unsigned long)inner_size,
             (unsigned long)outer_size - off);

    if (outer) {
        print_impl_t(p, outer, outer_size, off, off + inner_size);
    }
}

} // namespace

void print_memory(const uint8_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const uint16_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const uint32_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const uint64_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const int8_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const int16_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const int32_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const int64_t* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const float* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory(const double* data, size_t size) {
    print_memory_t(data, size);
}

void print_memory_slice(const uint8_t* inner,
                        size_t inner_size,
                        const uint8_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const uint16_t* inner,
                        size_t inner_size,
                        const uint16_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const uint32_t* inner,
                        size_t inner_size,
                        const uint32_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const uint64_t* inner,
                        size_t inner_size,
                        const uint64_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const int8_t* inner,
                        size_t inner_size,
                        const int8_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const int16_t* inner,
                        size_t inner_size,
                        const int16_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const int32_t* inner,
                        size_t inner_size,
                        const int32_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const int64_t* inner,
                        size_t inner_size,
                        const int64_t* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const float* inner,
                        size_t inner_size,
                        const float* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

void print_memory_slice(const double* inner,
                        size_t inner_size,
                        const double* outer,
                        size_t outer_size) {
    print_memory_slice_t(inner, inner_size, outer, outer_size);
}

} // namespace core
} // namespace roc
