/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_TEST_HELPERS_SENDER_H_
#define ROC_PUBLIC_API_TEST_HELPERS_SENDER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/context.h"
#include "test_helpers/utils.h"

#include "roc_core/array.h"
#include "roc_core/atomic.h"
#include "roc_core/noop_arena.h"
#include "roc_core/panic.h"
#include "roc_core/thread.h"

#include "roc/context.h"
#include "roc/endpoint.h"
#include "roc/sender.h"

namespace roc {
namespace api {
namespace test {

class Sender : public core::Thread {
public:
    Sender(Context& context,
           roc_sender_config& config,
           float sample_step,
           size_t num_chans,
           size_t frame_size,
           unsigned flags)
        : sndr_(NULL)
        , conn_metrics_(core::NoopArena)
        , sample_step_(sample_step)
        , num_chans_(num_chans)
        , frame_samples_(frame_size * num_chans)
        , flags_(flags)
        , stopped_(false) {
        CHECK(roc_sender_open(context.get(), &config, &sndr_) == 0);
        CHECK(sndr_);
    }

    ~Sender() {
        CHECK(roc_sender_close(sndr_) == 0);
    }

    void connect(const roc_endpoint* receiver_source_endp,
                 const roc_endpoint* receiver_repair_endp,
                 const roc_endpoint* receiver_control_endp,
                 roc_slot slot = ROC_SLOT_DEFAULT) {
        if ((flags_ & FlagRS8M) || (flags_ & FlagLDPC)) {
            CHECK(receiver_source_endp);
            CHECK(receiver_repair_endp);
            CHECK(roc_sender_connect(sndr_, slot, ROC_INTERFACE_AUDIO_SOURCE,
                                     receiver_source_endp)
                  == 0);
            CHECK(roc_sender_connect(sndr_, slot, ROC_INTERFACE_AUDIO_REPAIR,
                                     receiver_repair_endp)
                  == 0);
        } else {
            CHECK(receiver_source_endp);
            CHECK(!receiver_repair_endp);
            CHECK(roc_sender_connect(sndr_, slot, ROC_INTERFACE_AUDIO_SOURCE,
                                     receiver_source_endp)
                  == 0);
        }

        if (flags_ & FlagRTCP) {
            CHECK(receiver_control_endp);
            CHECK(roc_sender_connect(sndr_, slot, ROC_INTERFACE_AUDIO_CONTROL,
                                     receiver_control_endp)
                  == 0);
        } else {
            CHECK(!receiver_control_endp);
        }
    }

    void query_metrics(size_t requested_conns, roc_slot slot = ROC_SLOT_DEFAULT) {
        CHECK(conn_metrics_.resize(requested_conns));

        n_conn_metrics_ = requested_conns;

        CHECK(roc_sender_query(sndr_, slot, &send_metrics_, conn_metrics_.data(),
                               &n_conn_metrics_)
              == 0);

        CHECK(n_conn_metrics_ <= requested_conns);
    }

    const roc_sender_metrics& send_metrics() const {
        return send_metrics_;
    }

    size_t conn_metrics_count() const {
        return n_conn_metrics_;
    }

    const roc_connection_metrics& conn_metrics(size_t n) const {
        CHECK(n < n_conn_metrics_);
        return conn_metrics_[n];
    }

    void stop() {
        stopped_ = true;
    }

private:
    virtual void run() {
        float send_buf[MaxBufSize];
        float sample_value = sample_step_;

        while (!stopped_) {
            for (size_t ns = 0; ns < frame_samples_; ns += num_chans_) {
                for (size_t nc = 0; nc < num_chans_; ++nc) {
                    send_buf[ns + nc] = sample_value;
                }
                sample_value = increment_sample_value(sample_value, sample_step_);
            }

            roc_frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.samples = send_buf;
            frame.samples_size = frame_samples_ * sizeof(float);

            const int ret = roc_sender_write(sndr_, &frame);
            roc_panic_if_not(ret == 0);
        }
    }

    roc_sender* sndr_;

    roc_sender_metrics send_metrics_;
    core::Array<roc_connection_metrics, 16> conn_metrics_;
    size_t n_conn_metrics_;

    const float sample_step_;
    const size_t num_chans_;
    const size_t frame_samples_;
    const unsigned flags_;

    core::Atomic<int> stopped_;
};

} // namespace test
} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_TEST_HELPERS_SENDER_H_
