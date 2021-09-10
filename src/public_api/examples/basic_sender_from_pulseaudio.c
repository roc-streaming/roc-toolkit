/* Basic sender example.
 *
 * This example creates a sender and connects it to remote receiver.
 * Then it records audio stream from PulseAudio and writes it to the sender.
 *
 * Building:
 *   cc basic_sender_sine_wave.c -lroc
 *
 * Running:
 *   ./a.out
 *
 * License:
 *   public domain
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/endpoint.h>
#include <roc/log.h>
#include <roc/sender.h>

#include <pulse/simple.h>

/* Receiver parameters. */
#define MY_RECEIVER_IP "127.0.0.1"
#define MY_RECEIVER_SOURCE_PORT 10101
#define MY_RECEIVER_REPAIR_PORT 10102

/* Signal parameters */
#define MY_SAMPLE_RATE 44100
#define MY_NUM_CHANNELS 2
#define MY_BUFFER_SIZE 1000

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

int main() {
    /* Enable verbose logging. */
    roc_log_set_level(ROC_LOG_DEBUG);

    /* Initialize context config.
     * Initialize to zero to use default values for all fields. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    /* Create context.
     * Context contains memory pools and the network worker thread(s).
     * We need a context to create a sender. */
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Initialize sender config.
     * Initialize to zero to use default values for unset fields. */
    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    /* Setup input frame format. */
    sender_config.frame_sample_rate = MY_SAMPLE_RATE;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Use user-provided clock.
     * Sender will be clocked by PulseAudio source. Writer operation will be non-blocking.
     */
    sender_config.clock_source = ROC_CLOCK_EXTERNAL;

    /* Create sender. */
    roc_sender* sender = NULL;
    if (roc_sender_open(context, &sender_config, &sender) != 0) {
        oops();
    }

    /* Connect sender to the receiver source (audio) packets endpoint.
     * The receiver should expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on that port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP_RS8M_SOURCE);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_sender_connect(sender, ROC_INTERFACE_AUDIO_SOURCE, source_endp) != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Connect sender to the receiver repair (FEC) packets endpoint.
     * The receiver should expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on that port. */
    roc_endpoint* repair_endp = NULL;
    if (roc_endpoint_allocate(&repair_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(repair_endp, ROC_PROTO_RS8M_REPAIR);
    roc_endpoint_set_host(repair_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(repair_endp, MY_RECEIVER_REPAIR_PORT);

    if (roc_sender_connect(sender, ROC_INTERFACE_AUDIO_REPAIR, repair_endp) != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(repair_endp) != 0) {
        oops();
    }

    /* Initialize PulseAudio parameters. */
    pa_sample_spec sample_spec;
    memset(&sample_spec, 0, sizeof(sample_spec));
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = MY_SAMPLE_RATE;
    sample_spec.channels = MY_NUM_CHANNELS;

    /* Open PulseAudio stream. */
    pa_simple* simple = pa_simple_new(NULL, "example app", PA_STREAM_RECORD, NULL,
                                      "example stream", &sample_spec, NULL, NULL, NULL);
    if (!simple) {
        oops();
    }

    /* Read samples from PulseAudio and write them to the sender. */
    for (;;) {
        /* Read samples from PulseAudio
         * PulseAudio will block until the source can provide more samples. */
        float samples[MY_BUFFER_SIZE];
        if (pa_simple_read(simple, samples, sizeof(samples), NULL) != 0) {
            break;
        }

        /* Write samples to the sender. */
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_sender_write(sender, &frame) != 0) {
            oops();
        }
    }

    /* Destroy sender. */
    if (roc_sender_close(sender) != 0) {
        oops();
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
