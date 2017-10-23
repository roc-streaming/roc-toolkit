/* This file is part of Roc PulseAudio integration.
 *
 * Copyright 2017 Victor Gaydov
 * Copyright 2017 Mikhail Baranov
 *
 * Licensed under GNU Lesser General Public License 2.1 or any later version.
 */

/* config.h from pulseaudio directory (generated after ./configure) */
#include <config.h>

/* public pulseaudio headers */
#include <pulse/xmalloc.h>
#include <pulse/rtclock.h>

/* private pulseaudio headers */
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/sink.h>
#include <pulsecore/thread.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/log.h>

/* roc headers */
#include <roc/sender.h>

/* system headers */
#include <stdlib.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

PA_MODULE_AUTHOR("Victor Gaydov & Mikhail Baranov");
PA_MODULE_DESCRIPTION("Write samples using Roc sender");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "sink_name=<name for the sink> "
        "sink_properties=<properties for the sink> "
        "local_ip=<local sender ip> "
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
    roc_sender* sender;
};

static const char* const roc_sink_modargs[] = {
    "sink_name",
    "sink_properties",
    "local_ip",
    "remote_ip",
    "remote_source_port",
    "remote_repair_port",
    NULL
};

static int parse_address(struct sockaddr_in* addr, const char* ip, const char* port) {
    char* end = NULL;
    long port_num = strtol(port, &end, 10);
    if (port_num < 0 || port_num >= 65536 || !end || *end) {
        pa_log("invalid port: %s", port);
        return -1;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons((uint16_t)port_num);

    if (*ip) {
        if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
            pa_log("invalid ip: %s", ip);
            return -1;
        }
    } else {
        addr->sin_addr.s_addr = INADDR_ANY;
    }

    return 0;
}

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
        const char *buf = pa_memblock_acquire(chunk.memblock);

        const float *samples = (const float*)(buf + chunk.index);
        const size_t n_samples = chunk.length / pa_sample_size(&u->sink->sample_spec);

        /* write samples from memblock to roc transmitter */
        const ssize_t n = roc_sender_write(u->sender, samples, n_samples);
        if (n != (ssize_t)n_samples) {
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

    struct sockaddr_in local_addr;
    if (parse_address(&local_addr, pa_modargs_get_value(args, "local_ip", ""), "0")
        < 0) {
        pa_log("invalid local address");
        goto error;
    }

    struct sockaddr_in remote_source_addr;
    if (parse_address(&remote_source_addr, pa_modargs_get_value(args, "remote_ip", ""),
                      pa_modargs_get_value(args, "remote_source_port", ""))
        < 0) {
        pa_log("invalid remote address for source packets");
        goto error;
    }

    struct sockaddr_in remote_repair_addr;
    if (parse_address(&remote_repair_addr, pa_modargs_get_value(args, "remote_ip", ""),
                      pa_modargs_get_value(args, "remote_repair_port", ""))
        < 0) {
        pa_log("invalid remote address for repair packets");
        goto error;
    }

    roc_sender_config config;
    memset(&config, 0, sizeof(config));
    config.flags |= ROC_FLAG_DISABLE_INTERLEAVER;

    u->sender = roc_sender_new(&config);
    if (!u->sender) {
        pa_log("can't create roc sender");
        goto error;
    }

    if (roc_sender_bind(u->sender, (struct sockaddr*)&local_addr) != 0) {
        pa_log("can't bind roc sender to local address");
        goto error;
    }

    if (roc_sender_connect(u->sender, ROC_PROTO_RTP_RSM8_SOURCE,
                           (struct sockaddr*)&remote_source_addr)
        != 0) {
        pa_log("can't connect roc sender to remote address");
        goto error;
    }

    if (roc_sender_connect(u->sender, ROC_PROTO_RSM8_REPAIR,
                           (struct sockaddr*)&remote_repair_addr)
        != 0) {
        pa_log("can't connect roc sender to remote address");
        goto error;
    }

    if (roc_sender_start(u->sender) != 0) {
        pa_log("can't start roc sender");
        goto error;
    }

    /* create and initialize sink */
    pa_sink_new_data data;
    pa_sink_new_data_init(&data);
    data.driver = "roc_sink";
    data.module = m;
    pa_sink_new_data_set_name(
        &data,
        pa_modargs_get_value(args, "sink_name", "roc_sink"));
    pa_sink_new_data_set_sample_spec(&data, &sample_spec);
    pa_sink_new_data_set_channel_map(&data, &channel_map);

    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "Roc Sink");

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
    if (!(u->thread = pa_thread_new("roc_sink", thread_loop, u))) {
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
        roc_sender_stop(u->sender);
        roc_sender_delete(u->sender);
    }

    pa_xfree(u);
}
