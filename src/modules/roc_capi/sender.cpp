/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_capi/sender.h"

#include "roc_core/math.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_audio/sample_buffer_queue.h"
#include "roc_pipeline/client.h"
#include "roc_rtp/composer.h"
#include "roc_sndio/reader.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"

using namespace roc;

struct roc_sender
{
    roc_sender():
        client( sample_queue, trx.udp_sender(),
                trx.udp_composer(), rtp_composer,
                config)
    {}

    ~roc_sender(){}

    pipeline::ClientConfig config;
    audio::SampleBufferQueue sample_queue;
    rtp::Composer rtp_composer;

    audio::ISampleBufferPtr buffer;
    size_t buffer_pos;
    size_t n_bufs;

    netio::Transceiver trx;
    pipeline::Client client;
};

static size_t roc_sender_send_packet(
    roc_sender *sender, const float *samples, const size_t n_samples);

roc_sender* roc_sender_open(const char *destination_adress)
{
    datagram::Address src_addr;
    datagram::Address dst_addr;
    if (!netio::parse_address(destination_adress, dst_addr)) {
        roc_log(LOG_ERROR, "can't parse source address: %s", destination_adress);
        return NULL;
    }

    core::ScopedPtr<roc_sender> sender(new roc_sender());
    if (!sender) {
        return NULL;
    }

    sender->config = pipeline::ClientConfig();
    sender->config.options = 0;
    sender->config.options |= pipeline::EnableFEC;

    if (!sender->trx.add_udp_sender(src_addr)) {
        roc_log(LOG_ERROR, "can't register udp sender: %s",
                datagram::address_to_str(src_addr).c_str());
        return NULL;
    }

    sender->client.set_sender(src_addr);
    sender->client.set_receiver(dst_addr);

    sender->trx.start();
    sender->client.start();

    return sender.release();
}

size_t roc_sender_write(roc_sender *sender, const float *samples, size_t n_samples)
{
    size_t sent_samples = 0;

    while (sent_samples < n_samples) {
        size_t n = roc_sender_send_packet(
            sender, samples + sent_samples, n_samples - sent_samples);

        if (n == 0) {
            break;
        }

        sent_samples += n;
    }

    return sent_samples;
}

size_t roc_sender_send_packet(
    roc_sender *sender, const float *samples, size_t n_samples)
{
    audio::ISampleBufferComposer& composer = audio::default_buffer_composer();

    const size_t num_ch = 2;

    const size_t buffer_size = ROC_CONFIG_DEFAULT_SERVER_TICK_SAMPLES * num_ch;

    if (!sender->buffer) {
        if (!(sender->buffer = composer.compose())) {
            roc_log(LOG_ERROR, "reader: can't compose buffer");
            return 0;
        }

        if (buffer_size > sender->buffer->max_size()) {
            roc_panic(
                "reader:"
                " maximum buffer size should be at least n_channels * n_samples:"
                " decoder_bufsz=%lu, max_bufsz=%lu, n_channels=%lu",
                (unsigned long)buffer_size,        //
                (unsigned long)sender->buffer->max_size(), //
                (unsigned long)2);
        }

        sender->buffer->set_size(buffer_size);
    }

    packet::sample_t* buf_samples = sender->buffer->data();

    const size_t samples_2_copy =
                ROC_MIN(sender->buffer->size()-sender->buffer_pos, n_samples);
    memcpy(&buf_samples[sender->buffer_pos], samples, samples_2_copy*sizeof(roc::packet::sample_t));
    sender->buffer_pos += samples_2_copy;
    samples += samples_2_copy;
    if (sender->buffer_pos == sender->buffer->size()) {
        sender->sample_queue.write(*sender->buffer);

        sender->buffer.reset();
        sender->buffer_pos = 0;

        sender->n_bufs++;
    }

    return samples_2_copy;
}

void roc_sender_close(roc_sender *sender)
{
    delete sender;
}

uint32_t roc_sender_get_latency(roc_sender *sender)
{
    (void)sender;
    return 0;
}
