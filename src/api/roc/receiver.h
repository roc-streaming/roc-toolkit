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

#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct roc_receiver_config { int TODO; } roc_receiver_config;

typedef struct roc_receiver roc_receiver;

roc_receiver* roc_receiver_new(const roc_receiver_config* config);

void roc_receiver_delete(roc_receiver* receiver);

bool roc_receiver_bind(roc_receiver* receiver, const char* address);

ssize_t roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples);

#ifdef __cplusplus
}
#endif

#endif // ROC_RECEIVER_H_
