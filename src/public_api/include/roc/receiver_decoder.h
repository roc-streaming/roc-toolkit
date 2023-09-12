/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/receiver_decoder.h
 * \brief Roc receiver decoder.
 */

#ifndef ROC_RECEIVER_DECODER_H_
#define ROC_RECEIVER_DECODER_H_

#include "roc/config.h"
#include "roc/context.h"
#include "roc/frame.h"
#include "roc/metrics.h"
#include "roc/packet.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Receiver decoder node.
 *
 * Receiver decoder gets an encoded network packets from the user, decodes audio stream
 * from them, and provides it back to the user.
 *
 * Receiver decoder is a simplified networkless version of \ref roc_receiver. It
 * implements the same pipeline, but instead of receiving packets, it just gets them
 * from the user. The user is responsible for delivering these packets to receiver.
 *
 * For detailed description of receiver pipeline, see documentation for \ref roc_receiver.
 *
 * **Life cycle**
 *
 * - Decoder is created using roc_receiver_decoder_open().
 *
 * - The user activates one or more interfaces by invoking
 *   roc_receiver_decoder_activate(). This tells decoder what types of streams to consume
 *   and what protocols to use for them (e.g. only audio packets or also redundancy
 *   packets).
 *
 * - The per-interface streams of encoded packets are iteratively pushed to the decoder
 *   using roc_receiver_decoder_push().
 *
 * - The audio stream is iteratively popped from the decoder using
 *   roc_receiver_decoder_pop(). User should push all available packets to all
 *   interfaces before popping a frame.
 *
 * - User is responsible for delivering packets from \ref roc_sender_encoder and pushing
 *   them to appropriate interfaces of decoder.
 *
 * - The receiver is eventually destroyed using roc_receiver_decoder_close().
 *
 * **Interfaces and protocols**
 *
 * Receiver decoder may have one or several *interfaces*, as defined in \ref
 * roc_interface. The interface defines the type of the communication with the remote node
 * and the set of the protocols supported by it.
 *
 * Each interface has its own packet queue. When a packet is pushed to the decoder, it
 * is accumulated in the queue. When a frame is popped from the decoder, it consumes
 * those accumulated packets.
 *
 * **Thread safety**
 *
 * Can be used concurrently.
 */
typedef struct roc_receiver_decoder roc_receiver_decoder;

/** Open a new decoder.
 *
 * Allocates and initializes a new decoder, and attaches it to the context.
 *
 * **Parameters**
 *  - \p context should point to an opened context
 *  - \p config should point to an initialized config
 *  - \p result should point to an unitialized roc_receiver_decoder pointer
 *
 * **Returns**
 *  - returns zero if the decoder was successfully created
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p config; it may be safely deallocated
 *    after the function returns
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_receiver_decoder_close() to free it
 *  - attaches created decoder to \p context; the user should not close context
 *    before closing decoder
 */
ROC_API int roc_receiver_decoder_open(roc_context* context,
                                      const roc_receiver_config* config,
                                      roc_receiver_decoder** result);

/** Activate decoder interface.
 *
 * Checks that the protocol is valid and supported by the interface, and
 * initializes given interface with given protocol.
 *
 * The user should invoke roc_receiver_decoder_push() for all activated interfaces
 * and deliver packets from appropriate interfaces of \ref roc_sender_encoder.
 *
 * **Parameters**
 *  - \p decoder should point to an opened decoder
 *  - \p iface specifies the decoder interface
 *  - \p proto specifies the decoder protocol
 *
 * **Returns**
 *  - returns zero if interface was successfully activated
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 */
ROC_API int roc_receiver_decoder_activate(roc_receiver_decoder* decoder,
                                          roc_interface iface,
                                          roc_protocol proto);

/** Query decoder metrics.
 *
 * Reads decoder metrics into provided struct.
 *
 * To retrieve per-session metrics, set \c sessions field of \ref roc_receiver_metrics
 * to a buffer of \ref roc_session_metrics structs, and \c sessions_size to the number
 * of structs in buffer. The function will write session metrcis to the buffer and
 * update \c sessions_size with the actual number of sessions written.
 *
 * If \c sessions_size is lesser than actual number of sessions, metrics for some
 * sessions will be dropped. \c num_sessions will always contain actual total number.
 *
 * If \c sessions field is NULL, per-session metrics are not retrieved.
 *
 * **Parameters**
 *  - \p decoder should point to an opened decoder
 *  - \p metrics specifies struct where to write metrics
 *
 * **Returns**
 *  - returns zero if the slot was successfully removed
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value if the slot does not exist
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p metrics or its \c sessions field; they
 *    may be safely deallocated after the function returns
 */
ROC_API int roc_receiver_decoder_query(roc_receiver_decoder* decoder,
                                       roc_receiver_metrics* metrics);

/** Write packet to decoder.
 *
 * Add encoded packet to the interface queue.
 *
 * The user should iteratively push all delivered packets to appropriate interfaces. They
 * will be later consumed by roc_receiver_decoder_pop().
 *
 * **Parameters**
 *  - \p decoder should point to an opened decoder
 *  - \p packet should point to an initialized packet; it should contain pointer to
 *    a buffer and it's size; the buffer is fully copied into decoder
 *
 * **Returns**
 *  - returns zero if a packet was successfully copied to decoder
 *  - returns a negative value if the interface is not activated
 *  - returns a negative value if the buffer size of the provided packet is too large
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p packet; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_receiver_decoder_push(roc_receiver_decoder* decoder,
                                      roc_interface iface,
                                      const roc_packet* packet);

/** Read samples from decoder.
 *
 * Reads pushed network packets, decodes packets, repairs losses, extracts samples,
 * adjusts sample rate and channel layout, compensates clock drift, and stores
 * samples into the provided frame.
 *
 * If \ref ROC_CLOCK_SOURCE_INTERNAL is used, the function blocks until it's time to
 * decode the samples according to the configured sample rate.
 *
 * Until at least one interface is activated, decoder produces silence.
 *
 * **Parameters**
 *  - \p decoder should point to an opened decoder
 *  - \p frame should point to an initialized frame; it should contain pointer to
 *    a buffer and it's size; the buffer is fully filled with data from decoder
 *
 * **Returns**
 *  - returns zero if all samples were successfully decoded
 *  - returns a negative value if the arguments are invalid
 *  - returns a negative value on resource allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p frame; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_receiver_decoder_pop(roc_receiver_decoder* decoder, roc_frame* frame);

/** Close decoder.
 *
 * Deinitializes and deallocates the decoder, and detaches it from the context. The user
 * should ensure that nobody uses the decoder during and after this call. If this
 * function fails, the decoder is kept opened and attached to the context.
 *
 * **Parameters**
 *  - \p decoder should point to an opened decoder
 *
 * **Returns**
 *  - returns zero if the decoder was successfully closed
 *  - returns a negative value if the arguments are invalid
 *
 * **Ownership**
 *  - ends the user ownership of \p decoder; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_receiver_decoder_close(roc_receiver_decoder* decoder);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_RECEIVER_DECODER_H_ */
