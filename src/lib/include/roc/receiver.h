/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/receiver.h
 * @brief Roc receiver.
 */

#ifndef ROC_RECEIVER_H_
#define ROC_RECEIVER_H_

#include "roc/address.h"
#include "roc/config.h"
#include "roc/context.h"
#include "roc/frame.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Roc receiver.
 * TODO.
 */
typedef struct roc_receiver roc_receiver;

/** Open a new receiver.
 * TODO.
 */
ROC_API roc_receiver* roc_receiver_open(roc_context* context,
                                        const roc_receiver_config* config);

/** Bind receiver to a local port.
 * TODO.
 */
ROC_API int
roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, roc_address* address);

/** Read samples from the receiver.
 * TODO.
 */
ROC_API int roc_receiver_read(roc_receiver* receiver, roc_frame* frame);

/** Close receiver.
 * TODO.
 */
ROC_API int roc_receiver_close(roc_receiver* receiver);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_RECEIVER_H_ */
