/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <unistd.h>

#include "roc_core/exit.h"

namespace roc {
namespace core {

void fast_exit(int code) {
    _exit(code);
}

} // namespace core
} // namespace roc
