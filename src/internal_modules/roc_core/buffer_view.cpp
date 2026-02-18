/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/buffer_view.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

BufferView::BufferView(void* data, size_t size)
    : size_(size)
    , data_(data) {
    roc_panic_if_msg(data == NULL, "buffer view: attempt to create view with null data");
    roc_panic_if_msg(size == 0, "buffer view: attempt to create view with zero size");
}

} // namespace core
} // namespace roc
