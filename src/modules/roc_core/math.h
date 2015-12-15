/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/math.h
//! @brief Math helpers.

#ifndef ROC_CORE_MATH_H_
#define ROC_CORE_MATH_H_

#include <cmath>

//! Get minimum value.
#define ROC_MIN(a, b) ((a) <= (b) ? (a) : (b))

//! Get maximum value.
#define ROC_MAX(a, b) ((a) >= (b) ? (a) : (b))

//! Get absolute value.
#define ROC_ABS(a) ((a) > 0 ? (a) : -(a))

#endif // ROC_CORE_MATH_H_
