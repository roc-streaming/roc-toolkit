/*
 * This file is part of Roc PulseAudio integration.
 *
 * Copyright (c) 2017 Roc authors
 *
 * Licensed under GNU Lesser General Public License 2.1 or any later version.
 */

/* config.h from pulseaudio directory (generated after ./configure) */
#include <config.h>

/* public pulseaudio headers */
#include <pulse/rtclock.h>
#include <pulse/xmalloc.h>

/* private pulseaudio headers */
#include <pulsecore/log.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/sink.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/thread.h>

/* roc headers */
#include <roc/context.h>
#include <roc/log.h>
#include <roc/sender.h>

/* local headers */
#include "module_helpers.h"

PA_MODULE_AUTHOR("Roc authors");
PA_MODULE_DESCRIPTION("Write samples using Roc sender");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "sink_name=<name for the sink> "
        "sink_properties=<properties for the sink> "
        "remote_ip=<remote receiver ip> "
        "remote_source_port=<remote receiver port for source packets> "
        "remote_repair_port=<remote receiver port for repair packets>");

struct roc_sink_userdata {
    pa_module* module;
    pa_sink* sink;

    pa_rtpoll* rtpoll;
    pa_thread* thread;
    pa_thread_mq thread_mq;

    uint64_t rendered_bytes;

    roc_endpoint* remote_source_endp;
    roc_endpoint* remote_repair_endp;

    roc_context* context;
    roc_sender* sender;
};

static const char* const roc_sink_modargs[] = {
    "sink_name",
    "sink_properties",
    "remote_ip",
    "remote_source_port",
    "remote_repair_port",
    NULL
};

static int process_message(
    pa_msgobject* o, int code, void* data, int64_t offset, pa_memchunk* chunk) {
    switch (code) {
    case PA_SINK_MESSAGE_GET_LATENCY:
        /* TODO: we should report internal latency here */
        *((pa_usec_t*)data) = 0;
        return 0;
    }

    return pa_sink_process_msg(o, code, data, offset, chunk);
}

static void process_samples(struct roc_sink_userdata* u, uint64_t expected_bytes) {
    pa_assert(u);

    while (u->rendered_bytes < expected_bytes) {
        /* read chunk from every connected sink input, mix them, allocate
         * memblock, fill it with mixed samples, and return it to us.
         */
        pa_memchunk chunk;
        pa_sink_render(u->sink, 0, &chunk);

        /* start reading chunk's memblock */
        char *buf = pa_memblock_acquire(chunk.memblock);

        /* prepare audio frame */
        roc_frame frame;
        memset(&frame, 0, sizeof(frame));

        frame.samples = buf + chunk.index;
        frame.samples_size = chunk.length;

        /* write samples from memblock to roc transmitter */
        if (roc_sender_write(u->sender, &frame) != 0) {
            break;
        }

        u->rendered_bytes += chunk.length;

        /* finish reading memblock */
        pa_memblock_release(chunk.memblock);

        /* return memblock to the pool */
        pa_memblock_unref(chunk.memblock);
    }

}

static void process_rewind(struct roc_sink_userdata* u) {
    pa_assert(u);

    pa_sink_process_rewind(u->sink, 0);
}

static void process_error(struct roc_sink_userdata* u) {
    pa_assert(u);

    pa_asyncmsgq_post(
        u->thread_mq.outq,
        PA_MSGOBJECT(u->module->core),
        PA_CORE_MESSAGE_UNLOAD_MODULE,
        u->module,
        0,
        NULL,
        NULL);

    pa_asyncmsgq_wait_for(
        u->thread_mq.inq,
        PA_MESSAGE_SHUTDOWN);
}

static void thread_loop(void* arg) {
    struct roc_sink_userdata *u = arg;
    pa_assert(u);

    pa_thread_mq_install(&u->thread_mq);

    const pa_usec_t poll_interval = 10000;

    pa_usec_t start_time = 0;
    pa_usec_t next_time = 0;

    for (;;) {
        /* process rewind */
        if (u->sink->thread_info.rewind_requested) {
            process_rewind(u);
        }

        /* process sink inputs */
        if (PA_SINK_IS_OPENED(u->sink->thread_info.state)) {
            pa_usec_t now_time = pa_rtclock_now();

            if (start_time == 0) {
                start_time = now_time;
                next_time = start_time + poll_interval;
            }
            else {
                while (now_time >= next_time) {
                    uint64_t expected_bytes =
                        pa_usec_to_bytes(next_time - start_time, &u->sink->sample_spec);

                    /* render samples from sink inputs and write them to output file */
                    process_samples(u, expected_bytes);

                    /* next tick */
                    next_time += poll_interval;
                }
            }

            /* schedule set next rendering tick */
            pa_rtpoll_set_timer_absolute(u->rtpoll, next_time);
        }
        else {
            /* sleep until state change */
            start_time = 0;
            next_time = 0;
            pa_rtpoll_set_timer_disabled(u->rtpoll);
        }

        /* process events and wait next rendering tick */
#if PA_CHECK_VERSION(5, 99, 0)
        int ret = pa_rtpoll_run(u->rtpoll);
#else
        int ret = pa_rtpoll_run(u->rtpoll, true);
#endif
        if (ret < 0) {
            pa_log("pa_rtpoll_run returned error");
            goto error;
        }

        if (ret == 0) {
            break;
        }
    }

    return;

error:
    process_error(u);
}

void pa__done(pa_module*);

int pa__init(pa_module* m) {
    pa_assert(m);

    /* setup logs */
    roc_log_set_level(ROC_LOG_DEBUG);
    roc_log_set_handler(rocpa_log_handler);

    /* prepare sample spec and channel map used in this sink */
    pa_sample_spec sample_spec;
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = 44100;
    sample_spec.channels = 2;

    pa_channel_map channel_map;
    pa_channel_map_init_stereo(&channel_map);

    /* get module arguments (key-value list passed to load-module) */
    pa_modargs *args;
    if (!(args = pa_modargs_new(m->argument, roc_sink_modargs))) {
        pa_log("failed to parse module arguments");
        goto error;
    }

    /* create and initialize module-specific data */
    struct roc_sink_userdata *u = pa_xnew0(struct roc_sink_userdata, 1);
    pa_assert(u);
    m->userdata = u;

    u->module = m;
    u->rtpoll = pa_rtpoll_new();
    pa_thread_mq_init(&u->thread_mq, m->core->mainloop, u->rtpoll);

    if (rocpa_parse_endpoint(&u->remote_source_endp, ROCPA_DEFAULT_SOURCE_PROTO, args,
                             "remote_ip", "", "remote_source_port",
                             ROCPA_DEFAULT_SOURCE_PORT)
        < 0) {
        goto error;
    }

    if (rocpa_parse_endpoint(&u->remote_repair_endp, ROCPA_DEFAULT_REPAIR_PROTO, args,
                             "remote_ip", "", "remote_repair_port",
                             ROCPA_DEFAULT_REPAIR_PORT)
        < 0) {
        goto error;
    }

    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));

    if (roc_context_open(&context_config, &u->context) < 0) {
        pa_log("can't create roc context");
        goto error;
    }

    roc_sender_config sender_config;
    memset(&sender_config, 0, sizeof(sender_config));

    sender_config.frame_sample_rate = 44100;
    sender_config.frame_channels = ROC_CHANNEL_SET_STEREO;
    sender_config.frame_encoding = ROC_FRAME_ENCODING_PCM_FLOAT;

    if (roc_sender_open(u->context, &sender_config, &u->sender) < 0) {
        pa_log("can't create roc sender");
        goto error;
    }

    if (roc_sender_connect(u->sender, ROC_INTERFACE_AUDIO_SOURCE, u->remote_source_endp)
        != 0) {
        pa_log("can't connect roc sender to remote address");
        goto error;
    }

    if (roc_sender_connect(u->sender, ROC_INTERFACE_AUDIO_REPAIR, u->remote_repair_endp)
        != 0) {
        pa_log("can't connect roc sender to remote address");
        goto error;
    }

    /* create and initialize sink */
    pa_sink_new_data data;
    pa_sink_new_data_init(&data);
    data.driver = "roc_sink";
    data.module = m;
    pa_sink_new_data_set_name(
        &data,
        pa_modargs_get_value(args, "sink_name", "roc_sender"));
    pa_sink_new_data_set_sample_spec(&data, &sample_spec);
    pa_sink_new_data_set_channel_map(&data, &channel_map);

    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "Roc Sender");

    if (pa_modargs_get_proplist(
            args,
            "sink_properties",
            data.proplist,
            PA_UPDATE_REPLACE) < 0) {
        pa_log("invalid sink properties");
        pa_sink_new_data_done(&data);
        goto error;
    }

    u->sink = pa_sink_new(m->core, &data, PA_SINK_LATENCY);
    pa_sink_new_data_done(&data);

    if (!u->sink) {
        pa_log("failed to create sink");
        goto error;
    }

    /* setup sink callbacks */
    u->sink->parent.process_msg = process_message;
    u->sink->userdata = u;

    /* setup sink event loop */
    pa_sink_set_asyncmsgq(u->sink, u->thread_mq.inq);
    pa_sink_set_rtpoll(u->sink, u->rtpoll);

    /* start thread for sink event loop and sample reader */
    if (!(u->thread = pa_thread_new("roc_sender", thread_loop, u))) {
        pa_log("failed to create thread");
        goto error;
    }

    pa_sink_put(u->sink);
    pa_modargs_free(args);

    return 0;

error:
    if (args) {
        pa_modargs_free(args);
    }
    pa__done(m);

    return -1;
}

void pa__done(pa_module* m) {
    pa_assert(m);

    struct roc_sink_userdata *u = m->userdata;
    if (!u) {
        return;
    }

    if (u->sink) {
        pa_sink_unlink(u->sink);
    }

    if (u->thread) {
        pa_asyncmsgq_send(u->thread_mq.inq, NULL, PA_MESSAGE_SHUTDOWN, NULL, 0, NULL);
        pa_thread_free(u->thread);
    }

    pa_thread_mq_done(&u->thread_mq);

    if (u->sink) {
        pa_sink_unref(u->sink);
    }

    if (u->rtpoll) {
        pa_rtpoll_free(u->rtpoll);
    }

    if (u->sender) {
        if (roc_sender_close(u->sender) != 0) {
            pa_log("failed to close roc sender");
        }
    }

    if (u->context) {
        if (roc_context_close(u->context) != 0) {
            pa_log("failed to close roc context");
        }
    }

    if (u->remote_source_endp) {
        if (roc_endpoint_deallocate(u->remote_source_endp) != 0) {
            pa_log("failed to deallocate roc endpoint");
        }
    }

    if (u->remote_repair_endp) {
        if (roc_endpoint_deallocate(u->remote_repair_endp) != 0) {
            pa_log("failed to deallocate roc endpoint");
        }
    }

    pa_xfree(u);
}
