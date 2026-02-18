/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/context.h
 * \brief Shared context.
 */

#ifndef ROC_CONTEXT_H_
#define ROC_CONTEXT_H_

#include "roc/config.h"
#include "roc/platform.h"
#include "roc/plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Shared context.
 *
 * Context contains memory pools and network worker threads, shared among objects attached
 * to the context. It is allowed both to create a separate context for every object, or
 * to create a single context shared between multiple objects.
 *
 * **Life cycle**
 *
 * A context is created using roc_context_open() and destroyed using roc_context_close().
 * Objects can be attached and detached to an opened context at any moment from any
 * thread. However, the user should ensure that the context is not closed until there
 * are no objects attached to the context.
 *
 * **Thread safety**
 *
 * Can be used concurrently
 *
 * \see roc_sender, roc_receiver
 */
typedef struct roc_context roc_context;

/** Open a new context.
 *
 * Allocates and initializes a new context. May start some background threads.
 * Overrides the provided \p result pointer with the newly created context.
 *
 * **Parameters**
 *  - \p config should point to an initialized config
 *  - \p result should point to an uninitialized roc_context pointer
 *
 * **Returns**
 *  - returns zero if the context was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are not enough resources
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_context_close() to free it
 */
ROC_API int roc_context_open(const roc_context_config* config, roc_context** result);

/** Register custom encoding.
 *
 * Registers \p encoding with given \p encoding_id. Registered encodings extend
 * built-in encodings defined by \ref roc_packet_encoding enum. Whenever you need to
 * specify packet encoding, you can use both built-in and registered encodings.
 *
 * On sender, you should register custom encoding and set to \c packet_encoding field
 * of \ref roc_sender_config, if you need to force specific encoding of packets.
 *
 * On receiver, you should register custom encoding with same id and specification,
 * if you did so on sender, and you're not using any signaling protocol (like RTSP)
 * that is capable of automatic exchange of encoding information.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p encoding_id should be in range [\c ROC_ENCODING_ID_MIN; \c ROC_ENCODING_ID_MAX]
 *  - \p encoding should point to valid encoding specification
 *
 * **Returns**
 *  - returns zero if encoding was successfully registered
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if encoding with given identifier already exists
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p encoding; copies its contents
 *    to internal encodings table
 */
ROC_API int roc_context_register_encoding(roc_context* context,
                                          int encoding_id,
                                          const roc_media_encoding* encoding);

/** Register custom PLC backend.
 *
 * Registers plugin that implements custom PLC backend. Registered backends extend
 * built-in PLC backends defined by \ref roc_plc_backend enum. Whenever you need to
 * specify PLC backend, you can use both built-in and registered backends.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p plugin_id should be in range [\c ROC_PLUGIN_ID_MIN; \c ROC_PLUGIN_ID_MAX]
 *  - \p plugin should point to plugin callback table
 *
 * **Returns**
 *  - returns zero if plugin was successfully registered
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if plugin with given identifier already exists
 *
 * **Ownership**
 *  - stores \p plugin pointer internally for later use; \p plugin should remain valid
 *    until \p context is closed
 */
ROC_API int
roc_context_register_plc(roc_context* context, int plugin_id, roc_plugin_plc* plugin);

/** Close the context.
 *
 * Stops any started background threads, deinitializes and deallocates the context.
 * The user should ensure that nobody uses the context during and after this call.
 *
 * If this function fails, the context is kept opened.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *
 * **Returns**
 *  - returns zero if the context was successfully closed
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if there are objects attached to the context
 *
 * **Ownership**
 *  - ends the user ownership of \p context; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_context_close(roc_context* context);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_CONTEXT_H_ */
