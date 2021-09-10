/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_ROOT_ALLOCATOR_H_
#define ROC_ROOT_ALLOCATOR_H_

#include "roc_core/heap_allocator.h"

namespace roc {
namespace api {

extern core::HeapAllocator root_allocator;

} // namespace api
} // namespace roc

#endif // ROC_ROOT_ALLOCATOR_H_
