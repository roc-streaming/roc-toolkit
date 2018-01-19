/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc/receiver.h
//! @brief Roc receiver.

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/address.h"
#include "roc/config.h"
#include "roc/context.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Receiver.
typedef struct roc_receiver roc_receiver;

//! Create a new receiver.
//! Returns a new object on success or NULL on error.
ROC_API roc_receiver* roc_receiver_open(roc_context* context,
                                        const roc_receiver_config* config);

//! Bind a new receiver port.
//! If the port is zero, an ephemeral port is selected and written back to addr.
//! Returns 0 on success or -1 on error.
ROC_API int
roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, roc_address* addr);

//! Read samples from receiver.
//! Returns positve number of samples on success or -1 on error.
ROC_API roc_ssize_t roc_receiver_read(roc_receiver* receiver,
                                      float* samples,
                                      roc_size_t n_samples);

//! Delete previously created receiver.
ROC_API int roc_receiver_close(roc_receiver* receiver);

#ifdef __cplusplus
}
#endif

#endif // ROC_RECEIVER_H_
