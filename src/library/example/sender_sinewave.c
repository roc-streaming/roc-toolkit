/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* Roc sender example.
 *
 * This example generates a 5-second sine wave and sends it to the receiver.
 * Receiver address and ports and other parameters are hardcoded.
 *
 * Building:
 *   gcc sender_sinewave.c -lroc
 *
 * Running:
 *   ./a.out
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/address.h>
#include <roc/context.h>
#include <roc/log.h>
#include <roc/sender.h>

/* Sender parameters. */
#define EXAMPLE_SENDER_IP "0.0.0.0"
#define EXAMPLE_SENDER_PORT 0

/* Receiver parameters. */
#define EXAMPLE_RECEIVER_IP "127.0.0.1"
#define EXAMPLE_RECEIVER_SOURCE_PORT 10001
#define EXAMPLE_RECEIVER_REPAIR_PORT 10002

/* Signal parameters */
#define EXAMPLE_SAMPLE_RATE 44100
#define EXAMPLE_SINE_RATE 440
#define EXAMPLE_SINE_SAMPLES (EXAMPLE_SAMPLE_RATE * 5)
#define EXAMPLE_BUFFER_SIZE 100

#define oops(msg)                                                                        \
    do {                                                                                 \
        fprintf(stderr, "oops: %s\n", msg);                                              \
        exit(1);                                                                         \
    } while (0)

static void gensine(float* samples, size_t num_samples) {
    double t = 0;
    size_t i;
    for (i = 0; i < num_samples / 2; i++) {
        const float s =
            (float)sin(2 * 3.14159265359 * EXAMPLE_SINE_RATE / EXAMPLE_SAMPLE_RATE * t);

        /* Fill samples for left and right channels. */
        samples[i * 2] = s;
        samples[i * 2 + 1] = -s;

        t += 1;
    }
}

int main() {
    /* Enable debug logging. */
    roc_log_set_level(ROC_LOG_DEBUG);

    /* Initialize context config.
     * Initialize to zero to use default values for all fields. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains memory pools and the network worker thread(s).
     * We need a context to create a sender. */
    roc_context* context = roc_context_open(&context_config);
    if (!context) {
        oops("roc_context_open");
    }

    /* Initialize sender config.
     * Initialize to zero to use default values for unset fields. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    /* Setup input frame format. */
    sender_config.frame_sample_rate = EXAMPLE_SAMPLE_RATE;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Turn on internal CPU timer.
     * Sender must send packets with steady rate, so we should either implement
     * clocking or ask the library to do so. We choose the second here. */
    sender_config.clock_source = ROC_CLOCK_INTERNAL;

    /* Create sender. */
    roc_sender* sender = roc_sender_open(context, &sender_config);
    if (!sender) {
        oops("roc_sender_open");
    }

    /* Bind sender to a random port. */
    roc_address sender_addr;
    if (roc_address_init(&sender_addr, ROC_AF_AUTO, EXAMPLE_SENDER_IP,
                         EXAMPLE_SENDER_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_bind(sender, &sender_addr) != 0) {
        oops("roc_sender_bind");
    }

    /* Connect sender to the receiver source (audio) packets port.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, EXAMPLE_RECEIVER_IP,
                         EXAMPLE_RECEIVER_SOURCE_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_SOURCE, ROC_PROTO_RTP_RS8M_SOURCE,
                           &recv_source_addr)
        != 0) {
        oops("roc_sender_connect");
    }

    /* Connect sender to the receiver repair (FEC) packets port.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, EXAMPLE_RECEIVER_IP,
                         EXAMPLE_RECEIVER_REPAIR_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_connect(sender, ROC_PORT_AUDIO_REPAIR, ROC_PROTO_RS8M_REPAIR,
                           &recv_repair_addr)
        != 0) {
        oops("roc_sender_connect");
    }

    /* Generate sine wave and write it to the sender. */
    size_t i;
    for (i = 0; i < EXAMPLE_SINE_SAMPLES / EXAMPLE_BUFFER_SIZE; i++) {
        /* Generate sine wave. */
        float samples[EXAMPLE_BUFFER_SIZE];
        gensine(samples, EXAMPLE_BUFFER_SIZE);

        /* Write samples to the sender. */
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = EXAMPLE_BUFFER_SIZE * sizeof(float);

        if (roc_sender_write(sender, &frame) != 0) {
            oops("roc_sender_write");
        }
    }

    /* Destroy sender. */
    if (roc_sender_close(sender) != 0) {
        oops("roc_sender_close");
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops("roc_context_close");
    }

    return 0;
}
