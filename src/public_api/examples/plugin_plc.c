/*
 * Register custom Packet loss concealment (PLC) plugin.
 *
 * PLC allows to reduce distortion caused by packet losses by replacing
 * gaps with interpolated data. It is used only when FEC wasn't able to
 * repair lost packets.
 *
 * Building:
 *   cc -o plugin_plc plugin_plc.c -lroc -lm
 *
 * Running:
 *   ./plugin_plc
 *
 * License:
 *   public domain
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/context.h>
#include <roc/log.h>
#include <roc/receiver.h>

/* Any number in range [ROC_PLUGIN_ID_MIN; ROC_PLUGIN_ID_MAX] */
#define MY_PLC_PLUGIN_ID ROC_PLUGIN_ID_MIN + 1

/* Audio parameters. */
#define MY_SAMPLE_RATE 44100
#define MY_CHANNEL_COUNT 2
#define MY_SINE_RATE 440

/* How much sample after a gap PLC needs for interpolation. */
#define MY_LOOKAHEAD_LEN_MS 100

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

/* PLC plugin instance.
 * roc_receiver will create an instance for every connection. */
struct my_plc {
    /* Here we could put state needed for interpolation. */
    unsigned int history_frame_counter;
    unsigned int lost_frame_counter;
    unsigned int sample_rate;
    unsigned int channel_count;
};

/* Create plugin instance. */
static void* my_plc_new(roc_plugin_plc* plugin, const roc_media_encoding* encoding) {
    printf("creating plc plugin instance\n");

    /* Note that sample rate and channel layout may have arbitrary values,
     * depending on the encoding used by the connection for which this
     * instance is created.
     *
     * Sample format is, however, always ROC_FORMAT_PCM_FLOAT32.
     */
    printf("using encoding:\n");
    printf(" sample_format = %u\n", encoding->format);
    printf(" sample_rate = %u\n", encoding->rate);
    printf(" channel_layout = %u\n", encoding->channels);

    struct my_plc* plc = calloc(1, sizeof(struct my_plc));

    plc->sample_rate = encoding->rate;

    switch (encoding->channels) {
    case ROC_CHANNEL_LAYOUT_MONO:
        plc->channel_count = 1;
        break;
    case ROC_CHANNEL_LAYOUT_STEREO:
        plc->channel_count = 2;
        break;
    default:
        printf("unsupported channel layout");
        free(plc);
        return NULL;
    }

    return plc;
}

/* Delete plugin instance. */
static void my_plc_delete(void* plugin_instance) {
    printf("deleting plc plugin instance\n");

    struct my_plc* plc = (struct my_plc*)plugin_instance;

    free(plc);
}

/* Get look-ahead length - how many samples after the lost frame
 * do we need for interpolation.
 * Returned value is measured as the number of samples per channel,
 * e.g. if sample rate is 44100Hz, length 4410 is 100ms */
static unsigned int my_plc_lookahead_len(void* plugin_instance) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    /* Convert milliseconds to number of samples. */
    return (unsigned int)(plc->sample_rate / 1000.0f * MY_LOOKAHEAD_LEN_MS);
}

/* Called when next frame is good (no loss). */
static void my_plc_process_history(void* plugin_instance,
                                   const roc_frame* history_frame) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    /* Here we can copy samples from history_frame to a ring buffer.
     * In this example we just history ignore frame.
     * Remember that history_frame will be invalidated after the callback
     * returns, so we'd need to do a deep copy if we want to use it later.
     */
    plc->history_frame_counter++;

#if 0
    /* Debug logs. In production code, it's not recommended to call functions like
     * printf() from processing callbacks, because they may block real-time
     * pipeline thread and cause priority inversion problems. You can either avoid
     * logging in processing callbacks or use a lock-free logger if you have one.
     */
    if (plc->history_frame_counter % 100 == 0) {
        printf("plc: history_frame_counter=%u lost_frame_counter=%u\n",
               plc->history_frame_counter, plc->lost_frame_counter);
    }
#endif
}

/* Called when next frame is lost and we must fill it with interpolated data.
 *
 * lost_frame is the frame to be filled (we must fill its buffer with the
 * interpolated samples)
 *
 * lookahead_frame contains samples going after the lost frame, which we can
 * use to improve interpolation results. Its size may vary from 0 to MY_LOOKAHEAD_SIZE.
 */
static void my_plc_process_loss(void* plugin_instance,
                                roc_frame* lost_frame,
                                const roc_frame* lookahead_frame) {
    struct my_plc* plc = (struct my_plc*)plugin_instance;

    /* Here we can implement interpolation. In this example we just fill lost frame
     * with a sine wave, thus we turn a loss into a beep.
     *
     * PLC plugin always uses ROC_FORMAT_PCM_FLOAT32, so we cast samples to float.
     *
     * PLC plugin may be asked to use arbitrary sample rate and channel layout,
     * so we use plc->sample_rate and plc->channel_count instead of
     * MY_SAMPLE_RATE and MY_CHANNEL_COUNT.
     */
    float* lost_samples = lost_frame->samples;
    size_t lost_sample_count =
        lost_frame->samples_size / sizeof(float) / plc->channel_count;

    for (unsigned ns = 0; ns < lost_sample_count; ns++) {
        const float s =
            (float)sin(2 * 3.14159265359 * MY_SINE_RATE / plc->sample_rate * ns) * 0.1f;

        for (unsigned nc = 0; nc < plc->channel_count; nc++) {
            *lost_samples++ = s;
        }
    }

    plc->lost_frame_counter++;
}

int main() {
    roc_log_set_level(ROC_LOG_INFO);

    /* Create context. */
    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    roc_context* context = NULL;
    if (roc_context_open(&context_config, &context) != 0) {
        oops();
    }

    /* Register plugin. */
    roc_plugin_plc plc_plugin;
    memset(&plc_plugin, 0, sizeof(plc_plugin));

    plc_plugin.new_cb = &my_plc_new;
    plc_plugin.delete_cb = &my_plc_delete;
    plc_plugin.lookahead_len_cb = &my_plc_lookahead_len;
    plc_plugin.process_history_cb = &my_plc_process_history;
    plc_plugin.process_loss_cb = &my_plc_process_loss;

    if (roc_context_register_plc(context, MY_PLC_PLUGIN_ID, &plc_plugin) != 0) {
        oops();
    }

    /* Prepare receiver config. */
    roc_receiver_config receiver_config;
    memset(&receiver_config, 0, sizeof(receiver_config));

    /* Setup frame encoding that we read from receiver.
     * Note that this encoding is different from the encoding used by PLC plugin. */
    receiver_config.frame_encoding.format = ROC_FORMAT_PCM;
    receiver_config.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
    receiver_config.frame_encoding.rate = MY_SAMPLE_RATE;
    receiver_config.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

    /* Enable PLC plugin. */
    receiver_config.plc_backend = (roc_plc_backend)MY_PLC_PLUGIN_ID;

    /* Create receiver. */
    roc_receiver* receiver = NULL;
    if (roc_receiver_open(context, &receiver_config, &receiver) != 0) {
        oops();
    }

    /*
     * Here we can run receiver loop.
     */

    /* Destroy receiver. */
    if (roc_receiver_close(receiver) != 0) {
        oops();
    }

    /* Destroy context.
     * Note that registered plugin must remain valid until this point. */
    if (roc_context_close(context) != 0) {
        oops();
    }

    return 0;
}
