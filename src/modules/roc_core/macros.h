/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/macros.h
//! @brief Macros.

#ifndef ROC_CORE_MACROS_H_
#define ROC_CORE_MACROS_H_

//! Get minimum value.
#define ROC_MIN(a, b) ((a) <= (b) ? (a) : (b))

//! Get maximum value.
#define ROC_MAX(a, b) ((a) >= (b) ? (a) : (b))

//! Get absolute value.
#define ROC_ABS(a) ((a) >= 0 ? (a) : -(a))

//! Subtruct b from a and convert to signed type.
#define ROC_UNSIGNED_SUB(signed_type, a, b) ((signed_type)((a) - (b)))

//! Return true if a is before b, taking wrapping into account.
#define ROC_UNSIGNED_LT(signed_type, a, b) (ROC_UNSIGNED_SUB(signed_type, a, b) < 0)

//! Return true if a is before b or a == b, taking wrapping into account.
#define ROC_UNSIGNED_LE(signed_type, a, b) (ROC_UNSIGNED_SUB(signed_type, a, b) <= 0)

//! Get number of elements in static array.
#define ROC_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

//! Cast a member of a structure out to the containing structure.
#define ROC_CONTAINER_OF(ptr, type, member)                                              \
    (reinterpret_cast<type*>((char*)(ptr)-offsetof(type, member)))

//! Stringize macro helper.
#define ROC_STRINGIZE_(s) #s

//! Stringize macro.
#define ROC_STRINGIZE(s) ROC_STRINGIZE_(s)

#endif // ROC_CORE_MACROS_H_
