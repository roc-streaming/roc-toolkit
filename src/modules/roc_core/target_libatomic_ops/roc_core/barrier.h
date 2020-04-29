/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libatomic_ops/roc_core/barrier.h
//! @brief Barrier integer.

#ifndef ROC_CORE_BARRIER_H_
#define ROC_CORE_BARRIER_H_

#include <atomic_ops.h>

//! Full memory barrier.
#define roc_barrier() AO_nop_full()

#endif // ROC_CORE_BARRIER_H_
