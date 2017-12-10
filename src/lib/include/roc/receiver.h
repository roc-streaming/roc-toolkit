/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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

//! Receiver.
typedef struct roc_receiver roc_receiver;

//! Create a new receiver.
//! This function allocates memory, but the receiver is not started.
//! Returns a new object on success or NULL on error.
ROC_API roc_receiver* roc_receiver_new(const roc_receiver_config* config);

//! Bind a new receiver port.
//! If the port is zero, an ephemeral port is selected and written back to addr.
//! Returns 0 on success or -1 on error.
ROC_API int
roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, struct sockaddr* addr);

//! Start the receiver.
ROC_API int roc_receiver_start(roc_receiver* receiver);

//! Read samples from receiver.
//! Returns positve number of samples on success or -1 on error.
ROC_API ssize_t roc_receiver_read(roc_receiver* receiver,
                                  float* samples,
                                  const size_t n_samples);

//! Stop the receiver.
ROC_API void roc_receiver_stop(roc_receiver* receiver);

//! Delete previously created receiver.
//! Receiver should be stopped.
ROC_API void roc_receiver_delete(roc_receiver* receiver);

#ifdef __cplusplus
}
#endif

#endif // ROC_RECEIVER_H_
