/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include "roc_core/math.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_audio/sample_buffer_queue.h"
#include "roc_pipeline/client.h"
#include "roc_rtp/composer.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"

using namespace roc;

namespace {

bool make_client_config(pipeline::ClientConfig& cc, const roc_sender_config* sc) {
    (void)sc;

    cc = pipeline::ClientConfig(pipeline::EnableInterleaving | pipeline::EnableFEC);

    return true;
}

} // anonymous

struct roc_sender {
    roc_sender(const pipeline::ClientConfig& config)
        : buffer_pos_(0)
        , n_bufs_(0)
        , client_(sample_queue_,
                  trx_.udp_sender(),
                  trx_.udp_composer(),
                  rtp_composer_,
                  config) {
    }

    ~roc_sender() {
        sample_queue_.write(audio::ISampleBufferConstSlice());

        client_.join();

        trx_.stop();
        trx_.join();
    }

    bool bind(const char* address) {
        datagram::Address src_addr;
        datagram::Address dst_addr;
        if (!netio::parse_address(address, dst_addr)) {
            roc_log(LogError, "can't parse source address: %s", address);
            return false;
        }

        if (!trx_.add_udp_sender(src_addr)) {
            roc_log(LogError, "can't register udp sender: %s",
                    datagram::address_to_str(src_addr).c_str());
            return false;
        }

        client_.set_sender(src_addr);
        client_.set_receiver(dst_addr);

        trx_.start();
        client_.start();

        return true;
    }

    ssize_t write(const float* samples, size_t n_samples) {
        size_t sent_samples = 0;

        while (sent_samples < n_samples) {
            size_t n = write_packet_(samples + sent_samples, n_samples - sent_samples);
            if (n == 0) {
                break;
            }
            sent_samples += n;
        }

        return (ssize_t)sent_samples;
    }

private:
    size_t write_packet_(const float* samples, size_t n_samples) {
        audio::ISampleBufferComposer& composer = audio::default_buffer_composer();

        const size_t num_ch = 2;
        const size_t buffer_size = ROC_CONFIG_DEFAULT_SERVER_TICK_SAMPLES * num_ch;

        if (!buffer_) {
            if (!(buffer_ = composer.compose())) {
                roc_log(LogError, "reader: can't compose buffer");
                return 0;
            }

            buffer_->set_size(buffer_size);
        }

        packet::sample_t* buf_samples = buffer_->data();

        const size_t samples_2_copy = ROC_MIN(buffer_->size() - buffer_pos_, n_samples);

        memcpy(&buf_samples[buffer_pos_], samples,
               samples_2_copy * sizeof(packet::sample_t));

        buffer_pos_ += samples_2_copy;
        samples += samples_2_copy;

        if (buffer_pos_ == buffer_->size()) {
            sample_queue_.write(*buffer_);

            buffer_.reset();
            buffer_pos_ = 0;

            n_bufs_++;
        }

        return samples_2_copy;
    }

    audio::SampleBufferQueue sample_queue_;
    rtp::Composer rtp_composer_;

    audio::ISampleBufferPtr buffer_;
    size_t buffer_pos_;
    size_t n_bufs_;

    netio::Transceiver trx_;
    pipeline::Client client_;
};

roc_sender* roc_sender_new(const roc_sender_config* sc) {
    pipeline::ClientConfig cc;

    if (!make_client_config(cc, sc)) {
        return NULL;
    }

    return new roc_sender(cc);
}

void roc_sender_delete(roc_sender* sender) {
    roc_panic_if(sender == NULL);

    delete sender;
}

bool roc_sender_bind(roc_sender* sender, const char* address) {
    roc_panic_if(sender == NULL);

    return sender->bind(address);
}

ssize_t
roc_sender_write(roc_sender* sender, const float* samples, const size_t n_samples) {
    roc_panic_if(sender == NULL);
    roc_panic_if(samples == NULL && n_samples != 0);

    return sender->write(samples, n_samples);
}
