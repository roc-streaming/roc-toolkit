/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc/context.h
//! @brief Roc context.

#ifndef ROC_CONTEXT_H_
#define ROC_CONTEXT_H_

#include "roc/config.h"
#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Context.
typedef struct roc_context roc_context;

//! Create a new context.
ROC_API roc_context* roc_context_open(const roc_context_config* config);

//! Start background thread.
ROC_API int roc_context_start(roc_context* context);

//! Terminate background thread and wait until it finishes.
ROC_API int roc_context_stop(roc_context* context);

//! Delete context.
ROC_API int roc_context_close(roc_context* context);

#ifdef __cplusplus
}
#endif

#endif // ROC_CONTEXT_H_
