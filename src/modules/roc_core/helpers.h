/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/helpers.h
//! @brief Helpers.

#ifndef ROC_CORE_HELPERS_H_
#define ROC_CORE_HELPERS_H_

#include "roc_core/stddefs.h"

//! Cast a member of a structure out to the containing structure.
#define ROC_CONTAINER_OF(ptr, type, member)                                              \
    (reinterpret_cast<type*>((char*)(ptr)-offsetof(type, member)))

//! Subtruct b from a and convert to signed type.
#define ROC_SUBTRACT(signed_type, a, b) ((signed_type)((a) - (b)))

//! Return true if a is before b, taking wrapping into account.
#define ROC_IS_BEFORE(signed_type, a, b) (ROC_SUBTRACT(signed_type, a, b) < 0)

//! Return true if a is before b or a == b, taking wrapping into account.
#define ROC_IS_BEFORE_EQ(signed_type, a, b) (ROC_SUBTRACT(signed_type, a, b) <= 0)

//! Stringize macro helper.
#define ROC_STRINGIZE_(s) #s

//! Stringize macro.
#define ROC_STRINGIZE(s) ROC_STRINGIZE_(s)

//! Get number of elements in static array.
#define ROC_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif // ROC_CORE_HELPERS_H_
