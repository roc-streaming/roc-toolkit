/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/memory_ops.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

void MemoryOps::poison_before_use(void* data, size_t size) {
    if (!data) {
        roc_panic("memory_ops: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_BeforeUse, size);
}

void MemoryOps::poison_after_use(void* data, size_t size) {
    if (!data) {
        roc_panic("memory_ops: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_AfterUse, size);
}

void MemoryOps::prepare_canary(void* data, size_t size) {
    if (!data) {
        roc_panic("memory_ops: data is null");
    }

    if (size == 0) {
        return;
    }
    memset(data, Pattern_Canary, size);
}

bool MemoryOps::check_canary(void* data, size_t size) {
    if (!data) {
        roc_panic("memory_ops: data is null");
    }

    if (size == 0) {
        return true;
    }
    char* data_begin = (char*)data;
    char* data_end = (char*)data + size;
    while (data_begin < data_end) {
        if (*data_begin != Pattern_Canary) {
            return false;
        }
        data_begin++;
    }
    return true;
}

} // namespace core
} // namespace roc
