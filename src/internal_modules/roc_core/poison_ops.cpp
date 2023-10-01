/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/poison_ops.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

void PoisonOps::before_use(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_BeforeUse, size);
}

void PoisonOps::after_use(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_AfterUse, size);
}

void PoisonOps::prepare_boundary_guard(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }
    memset(data, Pattern_BoundaryGuard, size);
}

void PoisonOps::check_boundary_guard(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }
    char* data_begin = (char*)data;
    char* data_end = (char*)data + size;
    while (data_begin < data_end) {
        if (*data_begin != Pattern_BoundaryGuard)
            roc_panic("poisoner: data is not boundary guard");
        data_begin++;
    }
}

} // namespace core
} // namespace roc
