/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/context.h
 * @brief Roc context.
 */

#ifndef ROC_CONTEXT_H_
#define ROC_CONTEXT_H_

#include "roc/config.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Roc context.
 *
 * Context contains memory allocator and pools, and the network thread. Objects that
 * need to perform memory allocations or network I/O are attached to a context to do
 * that. It is allowed both to create a separate context for every object, or to
 * create a single context shared between multiple objects.
 *
 * A context is created using roc_context_open() and destroyed using roc_context_close().
 * Objects can be attached and detached to an opened context at any moment from any
 * thread. However, the user should ensure that the context is not closed until there
 * are no objects attached to the context.
 *
 * After creating a context, the user should explicitly start the network thread using
 * roc_context_start() and eventually stop it using roc_context_stop(). It may be done
 * both before and after attaching objects to the context, however no network packets
 * will be sent and received until the thread is started.
 *
 * @b Thread-safety
 *  - can be used concurrently
 *
 * @see roc_sender, roc_receiver
 */
typedef struct roc_context roc_context;

/** Open a new context.
 *
 * Allocate and initialize a new context.
 *
 * @b Parameters
 *  - @p config defines context parameters. If @p config is NULL, default values are used
 *    for all parameters. Otherwise, default values are used for parameters set to zero.
 *
 * @b Returns
 *  - returns a new context if it was successfully created
 *  - returns NULL if the arguments are invalid
 *  - returns NULL if error occured
 */
ROC_API roc_context* roc_context_open(const roc_context_config* config);

/** Close context.
 *
 * Deinitialize and deallocate context. The user should ensure that nobody uses the
 * context since this function is called. If the network thread is running, it is
 * automatically stopped. If this function fails, the context is kept opened.
 *
 * @b Parameters
 *  - @p context should point to an opened context
 *
 * @b Returns
 *  - returns zero if the context was successfully closed
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are objects attached to the context
 */
ROC_API int roc_context_close(roc_context* context);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_CONTEXT_H_ */
