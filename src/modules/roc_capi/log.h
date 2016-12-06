/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief C API.

#ifndef ROC_LOG_H_
#define ROC_LOG_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* todo* enum */
void roc_log_set_level(const unsigned int verbosity);

#ifdef __cplusplus
}
#endif

#endif // ROC_LOG_H_
