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
 * Context contains memory pools and network worker thread(s). Other objects that work
 * with memory and network should be attached to a context. It is allowed both to create
 * a separate context for every object, or to create a single context shared between
 * multiple objects.
 *
 * A context is created using roc_context_open() and destroyed using roc_context_close().
 * Objects can be attached and detached to an opened context at any moment from any
 * thread. However, the user should ensure that the context is not closed until there
 * are no objects attached to the context.
 *
 * @b Thread-safety
 *  - can be used concurrently
 *
 * @see roc_sender, roc_receiver
 */
typedef struct roc_context roc_context;

/** Open a new context.
 *
 * Allocates and initializes a new context. May start some background threads.
 *
 * @b Parameters
 *  - @p config should point to an initialized config
 *
 * @b Returns
 *  - returns a new context if it was successfully created
 *  - returns NULL if the arguments are invalid
 *  - returns NULL if there are not enough resources
 */
ROC_API roc_context* roc_context_open(const roc_context_config* config);

/** Close the context.
 *
 * Stops any started background threads, deinitializes and deallocates the context.
 * The user should ensure that nobody uses the context during and after this call.
 *
 * If this function fails, the context is kept opened.
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
