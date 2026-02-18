/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/plugin.h
 * \brief User plugins.
 */

#ifndef ROC_PLUGIN_H_
#define ROC_PLUGIN_H_

#include "roc/config.h"
#include "roc/frame.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /** Minimum allowed packet encoding id.
     *
     * \ref ROC_ENCODING_ID_MIN and \ref ROC_ENCODING_ID_MAX define allowed
     * range for encoding identifiers registered by user.
     *
     * See \ref roc_context_register_encoding().
     */
    ROC_ENCODING_ID_MIN = 100,

    /** Maximum allowed packet encoding id.
     *
     * \ref ROC_ENCODING_ID_MIN and \ref ROC_ENCODING_ID_MAX define allowed
     * range for encoding identifiers registered by user.
     *
     * See \ref roc_context_register_encoding().
     */
    ROC_ENCODING_ID_MAX = 127
};

enum {
    /** Minimum allowed plugin id.
     *
     * \ref ROC_PLUGIN_ID_MIN and \ref ROC_PLUGIN_ID_MAX define allowed
     * range for plugin identifiers registered by user.
     *
     * See roc_context_register_plc().
     */
    ROC_PLUGIN_ID_MIN = 1000,

    /** Maximum allowed plugin id.
     *
     * \ref ROC_PLUGIN_ID_MIN and \ref ROC_PLUGIN_ID_MAX define allowed
     * range for plugin identifiers registered by user.
     *
     * See roc_context_register_plc().
     */
    ROC_PLUGIN_ID_MAX = 9999
};

/** PLC backend plugin.
 *
 * Packet loss concealment (PLC) is used to reduce distortion caused by lost packets
 * by filling gaps with interpolated or extrapolated data. It is used only when FEC
 * was not able to restore the packets.
 *
 * **Life cycle**
 *
 * PLC plugin is instantiated on receiver for every incoming connection from sender.
 *
 * This struct defines plugin callback table. For every connection, new_cb() is
 * invoked to create a new plugin instance, and then other callbacks are invoked on the
 * instance. When the connection is closed, delete_cb() is invoked to destroy instance.
 *
 * Multiple plugin instances may co-exist if there are multiple connections.
 *
 * **Workflow**
 *
 * When it's time to produce next frame (e.g. to be played on sound card), receiver calls
 * one of the two callbacks of the plugin instance:
 *
 *  - When the frame is successfully decoded from packet(s), receiver invokes
 *    process_history_cb(). Plugin may copy data from the frame and remember
 *    it for later use.
 *
 *  - When the frame is a gap caused by lost packet(s), receiver invokes
 *    process_loss_cb(). Plugin must fill the provided frame with the
 *    interpolated data.
 *
 * If lookahead_len_cb() returns non-zero, process_loss_cb() will be provided
 * with the frame following the lost one, if it is available.
 *
 * **Encoding**
 *
 * Media encoding used for frames passed to PLC plugin is determined dynamically.
 * When new_cb() is invoked, \c encoding argument defines what to use:
 *
 *  - \c rate and \c channels are the same as used in network packets of
 *    this particular connection; PLC plugin must be ready to work with
 *    arbitrary values, unless it's known that only certain packet encoding
 *    may be used by sender
 *
 *  - \c format is always \ref ROC_FORMAT_PCM and \c subformat is \ref
 *    ROC_SUBFORMAT_PCM_FLOAT32. Plugin doesn't need to support other formats.
 *
 * **Registration**
 *
 * PLC plugin should be registered using roc_context_register_plc() and then
 * enabled using \c plc_backend field of \ref roc_receiver_config.
 *
 * Plugin callback table is not copied, but is stored by reference inside \ref
 * roc_context. The callback table should remain valid and immutable until the
 * context is closed.
 *
 * **Thread-safety**
 *
 * Plugin callback table may be accessed from multiple threads concurrently and its
 * callbacks may be invoked concurrently. However, calls on the same plugin instance
 * returned from new_cb() are always serialized. Besides new_cb(), only calls
 * on different instances may happen concurrently.
 */
typedef struct roc_plugin_plc {
    /** Callback to create plugin instance.
     *
     * Invoked on receiver to create a plugin instance for a new connection.
     * Returned pointer is opaque. It is used as the argument to other callbacks.
     *
     * **Parameters**
     *  - \p plugin is a pointer to plugin callback table passed to
     *    roc_context_register_plc()
     *  - \p encoding defines encoding of the frames that will be passed to
     *    plugin callbacks
     */
    void* (*new_cb)(struct roc_plugin_plc* plugin, const roc_media_encoding* encoding);

    /** Callback to delete plugin instance.
     *
     * Invoked on receiver to destroy a plugin instance created by new_cb().
     *
     * **Parameters**
     *  - \p plugin_instance is a pointer to the instance returned by new_cb()
     */
    void (*delete_cb)(void* plugin_instance);

    /** Callback to obtain PLC look-ahead length, as number of samples per channel.
     *
     * Returned value defines how many samples following immediately after the lost frame
     * PLC wants to use for interpolation. See process_loss_cb() for details.
     *
     * **Parameters**
     *  - \p plugin_instance is a pointer to the instance returned by new_cb()
     */
    unsigned int (*lookahead_len_cb)(void* plugin_instance);

    /** Callback for frames without losses.
     *
     * Invoked on receiver when next frame was successfully decoded from packets.
     * If plugin wants to store frame for later use, it should copy its samples.
     *
     * The size of \p history_frame is arbitrary and may vary each call. The format of
     * the frame is defined by \c encoding argument of new_cb().
     *
     * **Parameters**
     *  - \p plugin_instance is a pointer to the instance returned by new_cb()
     *  - \p history_frame points to read-only frame with decoded data
     *
     * **Ownership**
     *  - frame and its data can't be used after the callback returns
     */
    void (*process_history_cb)(void* plugin_instance, const roc_frame* history_frame);

    /** Callback for frames with losses.
     *
     * Invoked on receiver when next frame is a gap caused by packet loss.
     * Plugin must fill \p lost_frame with the interpolated data.
     * Plugin must not change buffer and size of \p lost_frame, it is expected to
     * write samples into existing buffer.
     *
     * If lookahead_len_cb() returned non-zero length, \p lookahead_frame holds up
     * to that many samples, decoded from packets that follow the loss.
     * \p lookahead_frame may be shorter than look-ahead length and may be empty.
     * It's present only if packets following the loss happened to arrive early enough.
     *
     * The size of both frames is arbitrary and may vary each call. The format of the
     * frames is defined by \c encoding argument of new_cb().
     *
     * **Parameters**
     *  - \p plugin_instance is a pointer to the instance returned by new_cb()
     *  - \p lost_frame points to writable frame to be filled with the interpolation
     *  - \p lookahead_frame points to read-only frame following the lost one
     *
     * **Ownership**
     *  - frames and their data can't be used after the callback returns
     */
    void (*process_loss_cb)(void* plugin_instance,
                            roc_frame* lost_frame,
                            const roc_frame* lookahead_frame);
} roc_plugin_plc;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_PLUGIN_H_ */
