/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/global_destructor.h"
#include "roc_core/atomic_ops.h"

namespace roc {
namespace core {

namespace {

GlobalDestructor global_destructor;

int destructor_called = 0;

} // namespace

GlobalDestructor::~GlobalDestructor() {
    AtomicOps::store_seq_cst(destructor_called, true);
}

bool GlobalDestructor::is_destroying() {
    return AtomicOps::load_relaxed(destructor_called);
}

} // namespace core
} // namespace roc
