/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/attributes.h"

extern "C" ROC_EXPORT ROC_NOSANITIZE const char* __ubsan_default_options();

extern "C" ROC_EXPORT ROC_NOSANITIZE const char* __ubsan_default_options() {
    return "print_stacktrace=1";
}
