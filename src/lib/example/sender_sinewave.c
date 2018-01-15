/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <roc/context.h>
#include <roc/sender.h>

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
    /* Initialize context config.
     * We use default values here. */
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
    sender_config.flags |= ROC_FLAG_ENABLE_TIMER;

    /* Create sender. */
    roc_sender* sender = roc_sender_open(context, &sender_config);
    if (!sender) {
        oops("roc_sender_open");
    }

    /* Bind sender to a random port. */
    struct sockaddr_in sender_addr;
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sender_addr.sin_port = 0;

    if (roc_sender_bind(sender, (struct sockaddr*)&sender_addr) != 0) {
        oops("roc_sender_bind");
    }

    /* Connect sender to the receiver source (audio) packets port.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    struct sockaddr_in recv_source_addr;
    memset(&recv_source_addr, 0, sizeof(recv_source_addr));
    recv_source_addr.sin_family = AF_INET;
    recv_source_addr.sin_addr.s_addr = inet_addr(RECEIVER_IP);
    recv_source_addr.sin_port = htons(RECEIVER_SOURCE_PORT);

    if (roc_sender_connect(sender, ROC_PROTO_RTP_RSM8_SOURCE,
                           (struct sockaddr*)&recv_source_addr)
        != 0) {
        oops("roc_sender_connect");
    }

    /* Connect sender to the receiver repair (FEC) packets port.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    struct sockaddr_in recv_repair_addr;
    memset(&recv_repair_addr, 0, sizeof(recv_repair_addr));
    recv_repair_addr.sin_family = AF_INET;
    recv_repair_addr.sin_addr.s_addr = inet_addr(RECEIVER_IP);
    recv_repair_addr.sin_port = htons(RECEIVER_REPAIR_PORT);

    if (roc_sender_connect(sender, ROC_PROTO_RSM8_REPAIR,
                           (struct sockaddr*)&recv_repair_addr)
        != 0) {
        oops("roc_sender_connect");
    }

    /* Generate sine wave and write it to the sender. */
    double t = 0;
    size_t i, j;

    for (i = 0; i < NUM_SAMPLES / BUFFER_SIZE; i++) {
        float samples[BUFFER_SIZE];

        for (j = 0; j < BUFFER_SIZE / NUM_CHANNELS; j++) {
            float s = (float)sin(2 * PI * SINE_RATE / SAMPLE_RATE * t);

            /* Fill samples for left and right channel. */
            samples[j * 2] = s;
            samples[j * 2 + 1] = -s;

            t += 1;
        }

        /* Write samples to the sender. */
        if (roc_sender_write(sender, samples, BUFFER_SIZE) != BUFFER_SIZE) {
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
