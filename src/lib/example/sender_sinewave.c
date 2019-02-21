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
#define SENDER_IP "0.0.0.0"
#define SENDER_PORT 0

/* Receiver parameters. */
#define RECEIVER_IP "127.0.0.1"
#define RECEIVER_SOURCE_PORT 10001
#define RECEIVER_REPAIR_PORT 10002

/* Sender parameters */
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

/* Sine wave parameters. */
#define SINE_RATE 440
#define NUM_SAMPLES (SAMPLE_RATE * 5)
#define BUFFER_SIZE 100
#define PI 3.14159265359

#define oops(msg)                                                                        \
    do {                                                                                 \
        fprintf(stderr, "oops: %s\n", msg);                                              \
        exit(1);                                                                         \
    } while (0)

int main() {
    /* Enable debug logging. */
    roc_log_set_level(ROC_LOG_DEBUG);

    /* Initialize context config.
     * We use default values. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains global state like memory pools and the network loop thread.
     * We need a context to create a sender. */
    roc_context* context = roc_context_open(&context_config);
    if (!context) {
        oops("roc_context_open");
    }

    /* Start context thread. */
    if (roc_context_start(context) != 0) {
        oops("roc_context_start");
    }

    /* Initialize sender config. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    /* Turn on sender timing.
     * Sender must send packets with steady rate, so we should either implement
     * clocking or ask the library to do so. We choose the second here. */
    sender_config.automatic_timing = 1;

    /* Create sender. */
    roc_sender* sender = roc_sender_open(context, &sender_config);
    if (!sender) {
        oops("roc_sender_open");
    }

    /* Bind sender to a random port. */
    roc_address sender_addr;
    if (roc_address_init(&sender_addr, ROC_AF_AUTO, SENDER_IP, SENDER_PORT) != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_bind(sender, &sender_addr) != 0) {
        oops("roc_sender_bind");
    }

    /* Connect sender to the receiver source (audio) packets port.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    roc_address recv_source_addr;
    if (roc_address_init(&recv_source_addr, ROC_AF_AUTO, RECEIVER_IP,
                         RECEIVER_SOURCE_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_connect(sender, ROC_PROTO_RTP_RSM8_SOURCE, &recv_source_addr) != 0) {
        oops("roc_sender_connect");
    }

    /* Connect sender to the receiver repair (FEC) packets port.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_address recv_repair_addr;
    if (roc_address_init(&recv_repair_addr, ROC_AF_AUTO, RECEIVER_IP,
                         RECEIVER_REPAIR_PORT)
        != 0) {
        oops("roc_address_init");
    }
    if (roc_sender_connect(sender, ROC_PROTO_RSM8_REPAIR, &recv_repair_addr) != 0) {
        oops("roc_sender_connect");
    }

    /* Generate sine wave and write it to the sender. */
    double t = 0;
    size_t i, j;

    for (i = 0; i < NUM_SAMPLES / BUFFER_SIZE; i++) {
        float samples[BUFFER_SIZE];

        for (j = 0; j < BUFFER_SIZE / NUM_CHANNELS; j++) {
            float s = (float)sin(2 * PI * SINE_RATE / SAMPLE_RATE * t);

            /* Fill samples for left and right channels. */
            samples[j * 2] = s;
            samples[j * 2 + 1] = -s;

            t += 1;
        }

        /* Write samples to the sender. */
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.num_samples = BUFFER_SIZE;

        if (roc_sender_write(sender, &frame) != 0) {
            oops("roc_sender_write");
        }
    }

    /* Destroy sender. */
    if (roc_sender_close(sender) != 0) {
        oops("roc_sender_close");
    }

    /* Wait until all packets are sent and stop the context thread. */
    if (roc_context_stop(context) != 0) {
        oops("roc_context_stop");
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops("roc_context_close");
    }

    return 0;
}
