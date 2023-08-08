/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/poisoner.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

void Poisoner::before_use(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_BeforeUse, size);
}

void Poisoner::after_use(void* data, size_t size) {
    if (!data) {
        roc_panic("poisoner: data is null");
    }

    if (size == 0) {
        return;
    }

    memset(data, Pattern_AfterUse, size);
}

} // namespace core
} // namespace roc
