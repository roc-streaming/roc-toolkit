/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief C API.

#ifndef ROC_SENDER_H_
#define ROC_SENDER_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct roc_sender roc_sender;

roc_sender* roc_sender_open(const char *destination_adress);

size_t roc_sender_write(roc_sender *sender, const float *samples, size_t n_samples);

void roc_sender_close(roc_sender *sender);

//! Returns sum of all knnown latencies in microseconds.
//!
//! The Roc sums all latencies produced by every cause during its
//! work. For example:
//!   - length of the source packet on loading;
//!   - whole receivers latency which is available on SDP link.
//! @returns Returns the sum in microseconds. It doesn't indicate error.
uint32_t roc_sender_get_latency(roc_sender *sender);

#ifdef __cplusplus
}
#endif

#endif // ROC_SENDER_H_
