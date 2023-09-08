/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/macro_helpers.h
//! @brief Helper macros.

#ifndef ROC_CORE_MACRO_HELPERS_H_
#define ROC_CORE_MACRO_HELPERS_H_

#include "roc_core/stddefs.h"

//! Select minum value.
#define ROC_MIN(a, b) ((a) < (b) ? (a) : (b))

//! Select minum value.
#define ROC_MAX(a, b) ((a) > (b) ? (a) : (b))

//! Get number of elements in a static array.
#define ROC_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

//! Get minimum value of signed or unsigned integer type.
#define ROC_MIN_OF(type)                                                                 \
    ((type)-1 > (type)0 ? (type)0                                                        \
                        : (type)((~0ull) - ((1ull << ((sizeof(type) * 8) - 1)) - 1ull)))

//! Get maximum value of signed or unsigned integer type.
#define ROC_MAX_OF(type)                                                                 \
    ((type)-1 > (type)0 ? (type)(~0ull)                                                  \
                        : (type)((1ull << ((sizeof(type) * 8) - 1)) - 1ull))

//! Cast a member of a structure out to the containing structure.
#define ROC_CONTAINER_OF(ptr, type, member)                                              \
    (reinterpret_cast<type*>((char*)(ptr)-offsetof(type, member)))

//! Stringize macro helper.
#define ROC_STRINGIZE_(s) #s

//! Stringize macro.
#define ROC_STRINGIZE(s) ROC_STRINGIZE_(s)

#endif // ROC_CORE_MACRO_HELPERS_H_
