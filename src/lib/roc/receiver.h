/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief Roc receiver.

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/config.h"
#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct roc_receiver roc_receiver;

ROC_API roc_receiver* roc_receiver_new(const roc_config* config);

ROC_API void roc_receiver_delete(roc_receiver* receiver);

ROC_API bool roc_receiver_bind(roc_receiver* receiver, const char* address);

ROC_API ssize_t roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples);

#ifdef __cplusplus
}
#endif

#endif // ROC_RECEIVER_H_
