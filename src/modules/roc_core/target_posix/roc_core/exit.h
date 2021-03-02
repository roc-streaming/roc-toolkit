/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/exit.h
//! @brief Exit function.

#ifndef ROC_CORE_EXIT_H_
#define ROC_CORE_EXIT_H_

#include "roc_core/attributes.h"

namespace roc {
namespace core {

//! Terminate process immediately without calling destructors.
ROC_ATTR_NORETURN void fast_exit(int code);

} // namespace core
} // namespace roc

#endif // ROC_CORE_EXIT_H_
