/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/noop_arena.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

void* NoopArenaImpl::allocate(size_t size) {
    return NULL;
}

void NoopArenaImpl::deallocate(void* ptr) {
    roc_panic("noop arena: unexpected call");
}

size_t NoopArenaImpl::compute_allocated_size(size_t size) const {
    return 0;
}

size_t NoopArenaImpl::allocated_size(void* ptr) const {
    roc_panic("noop arena: unexpected call");
}

} // namespace core
} // namespace roc
