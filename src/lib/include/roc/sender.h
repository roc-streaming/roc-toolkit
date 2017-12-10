/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief Roc sender.

#ifndef ROC_SENDER_H_
#define ROC_SENDER_H_

#include "roc/config.h"
#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Sender.
typedef struct roc_sender roc_sender;

//! Create a new sender.
//! This function allocates memory, but the sender is not started.
//! Returns a new object on success or NULL on error.
ROC_API roc_sender* roc_sender_new(const roc_sender_config* config);

//! Bind sender to a local port.
//! If the port is zero, an ephemeral port is selected and written back to addr.
//! Returns 0 on success or -1 on error.
ROC_API int roc_sender_bind(roc_sender* sender, struct sockaddr* src_addr);

//! Connect sender to a remote port.
//! Returns 0 on success or -1 on error.
ROC_API int roc_sender_connect(roc_sender* sender,
                               roc_protocol proto,
                               const struct sockaddr* dst_addr);

//! Start the sender.
ROC_API int roc_sender_start(roc_sender* sender);

//! Write samples to sender.
//! Returns positve number of samples on success or -1 on error.
ROC_API ssize_t roc_sender_write(roc_sender* sender,
                                 const float* samples,
                                 const size_t n_samples);

//! Stop the sender.
ROC_API void roc_sender_stop(roc_sender* sender);

//! Delete previously created sender.
//! Sender should be stopped.
ROC_API void roc_sender_delete(roc_sender* sender);

#ifdef __cplusplus
}
#endif

#endif // ROC_SENDER_H_
