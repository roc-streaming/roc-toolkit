/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief C API.

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct roc_receiver roc_receiver;

roc_receiver* roc_receiver_open(const char *address);

ssize_t roc_receiver_read(roc_receiver *receiver, float *samples, const size_t n_samples);

void roc_receiver_close(roc_receiver *receiver);

//! Returns sum of all knnown latencies in microseconds.
//!
//! The Roc sums all latencies produced by every cause during its
//! work. For example:
//!   - length of the source packet on loading;
//!   - whole receivers latency which is available on SDP link.
//! @returns Returns the sum in microseconds. It doesn't indicate error.
uint32_t roc_receiver_get_latency(roc_receiver *receiver);

#ifdef __cplusplus
}
#endif

#endif // ROC_RECEIVER_H_
