/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/helpers.h
//! @brief Compile time helpers.

#ifndef ROC_CORE_HELPERS_H_
#define ROC_CORE_HELPERS_H_

//! Get number of elements in a static array.
#define ROC_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

//! Cast a member of a structure out to the containing structure.
#define ROC_CONTAINER_OF(ptr, type, member)                                              \
    (reinterpret_cast<type*>((char*)(ptr)-offsetof(type, member)))

//! Stringize macro helper.
#define ROC_STRINGIZE_(s) #s

//! Stringize macro.
#define ROC_STRINGIZE(s) ROC_STRINGIZE_(s)

#endif // ROC_CORE_HELPERS_H_
