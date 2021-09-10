/* Basic receiver example.
 *
 * This example creates a receiver and binds it to a known address.
 * Then it reads audio stream from the receiver and plays it using PulseAudio.
 *
 * Building:
 *   cc basic_receiver_to_pulseaudio.c -lroc -lpulse-simple
 *
 * Running:
 *   ./a.out
 *
 * License:
 *   public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/endpoint.h>
#include <roc/log.h>
#include <roc/receiver.h>

#include <pulse/simple.h>

/* Receiver parameters. */
#define MY_RECEIVER_IP "0.0.0.0"
#define MY_RECEIVER_SOURCE_PORT 10101
#define MY_RECEIVER_REPAIR_PORT 10102

/* Signal parameters. */
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
     * We need a context to create a receiver. */
    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Initialize receiver config.
     * We use default values. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Setup output frame format. */
    receiver_config.frame_sample_rate = MY_SAMPLE_RATE;
    receiver_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    receiver_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    /* Use user-provided clock.
     * Receiver will be clocked by PulseAudio sink. Read operation will be non-blocking.
     */
    receiver_config.clock_source = ROC_CLOCK_EXTERNAL;

    /* Create receiver. */
    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /* Bind receiver to the source (audio) packets endpoint.
     * The receiver will expect packets with RTP header and Reed-Solomon (m=8) FECFRAME
     * Source Payload ID on this port. */
    roc_endpoint* source_endp = NULL;
    if (roc_endpoint_allocate(&source_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(source_endp, ROC_PROTO_RTP_RS8M_SOURCE);
    roc_endpoint_set_host(source_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(source_endp, MY_RECEIVER_SOURCE_PORT);

    if (roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_SOURCE, source_endp) != 0) {
        oops();
    }

    if (roc_endpoint_deallocate(source_endp) != 0) {
        oops();
    }

    /* Bind receiver to the repair (FEC) packets endpoint.
     * The receiver will expect packets with Reed-Solomon (m=8) FECFRAME
     * Repair Payload ID on this port. */
    roc_endpoint* repair_endp = NULL;
    if (roc_endpoint_allocate(&repair_endp) != 0) {
        oops();
    }

    roc_endpoint_set_protocol(repair_endp, ROC_PROTO_RS8M_REPAIR);
    roc_endpoint_set_host(repair_endp, MY_RECEIVER_IP);
    roc_endpoint_set_port(repair_endp, MY_RECEIVER_REPAIR_PORT);

    if (roc_receiver_bind(receiver, ROC_INTERFACE_AUDIO_REPAIR, repair_endp) != 0) {
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
    pa_simple* simple = pa_simple_new(NULL, "example app", PA_STREAM_PLAYBACK, NULL,
                                      "example stream", &sample_spec, NULL, NULL, NULL);
    if (!simple) {
        oops();
    }

    /* Receive and play samples. */
    for (;;) {
        /* Read samples from receiver.
         * If not enough samples are received, receiver will pad buffer with zeros. */
        float samples[MY_BUFFER_SIZE];

        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = samples;
        frame.samples_size = sizeof(samples);

        if (roc_receiver_read(receiver, &frame) != 0) {
            oops();
        }

        /* Play samples.
         * PulseAudio will block until the sink can accept more samples. */
        if (pa_simple_write(simple, samples, sizeof(samples), NULL) != 0) {
            break;
        }
    }

    /* Wait until all samples are sent and played. */
    if (pa_simple_drain(simple, NULL) != 0) {
        oops();
    }

    /* Close PulseAudio stream. */
    pa_simple_free(simple);

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    /* Destroy context. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
