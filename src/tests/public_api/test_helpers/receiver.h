/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_LIBRARY_TEST_HELPERS_RECEIVER_H_
#define ROC_LIBRARY_TEST_HELPERS_RECEIVER_H_

#include <CppUTest/TestHarness.h>

#include <stdio.h>

#include "test_helpers/context.h"
#include "test_helpers/utils.h"

#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

#include "roc/config.h"
#include "roc/receiver.h"

namespace roc {
namespace library {
namespace test {

class Receiver {
public:
    Receiver(Context& context,
             roc_receiver_config& config,
             float sample_step,
             size_t frame_size,
             unsigned flags)
        : recv_(NULL)
        , source_endp_(NULL)
        , repair_endp_(NULL)
        , sample_step_(sample_step)
        , frame_size_(frame_size) {
        CHECK(roc_receiver_open(context.get(), &config, &recv_) == 0);
        CHECK(recv_);

        if (flags & FlagRS8M) {
            CHECK(roc_endpoint_allocate(&source_endp_) == 0);
            CHECK(roc_endpoint_set_uri(source_endp_, "rtp+rs8m://127.0.0.1:0") == 0);

            CHECK(roc_endpoint_allocate(&repair_endp_) == 0);
            CHECK(roc_endpoint_set_uri(repair_endp_, "rs8m://127.0.0.1:0") == 0);

            CHECK(roc_receiver_bind(recv_, ROC_INTERFACE_AUDIO_SOURCE, source_endp_)
                  == 0);
            CHECK(roc_receiver_bind(recv_, ROC_INTERFACE_AUDIO_REPAIR, repair_endp_)
                  == 0);
        } else if (flags & FlagLDPC) {
            CHECK(roc_endpoint_allocate(&source_endp_) == 0);
            CHECK(roc_endpoint_set_uri(source_endp_, "rtp+ldpc://127.0.0.1:0") == 0);

            CHECK(roc_endpoint_allocate(&repair_endp_) == 0);
            CHECK(roc_endpoint_set_uri(repair_endp_, "ldpc://127.0.0.1:0") == 0);

            CHECK(roc_receiver_bind(recv_, ROC_INTERFACE_AUDIO_SOURCE, source_endp_)
                  == 0);
            CHECK(roc_receiver_bind(recv_, ROC_INTERFACE_AUDIO_REPAIR, repair_endp_)
                  == 0);
        } else {
            CHECK(roc_endpoint_allocate(&source_endp_) == 0);
            CHECK(roc_endpoint_set_uri(source_endp_, "rtp://127.0.0.1:0") == 0);

            CHECK(roc_receiver_bind(recv_, ROC_INTERFACE_AUDIO_SOURCE, source_endp_)
                  == 0);
        }
    }

    ~Receiver() {
        if (source_endp_) {
            CHECK(roc_endpoint_deallocate(source_endp_) == 0);
        }

        if (repair_endp_) {
            CHECK(roc_endpoint_deallocate(repair_endp_) == 0);
        }

        CHECK(roc_receiver_close(recv_) == 0);
    }

    const roc_endpoint* source_endpoint() const {
        return source_endp_;
    }

    const roc_endpoint* repair_endpoint() const {
        return repair_endp_;
    }

    void run() {
        float rx_buff[MaxBufSize];

        size_t sample_num = 0;
        size_t frame_num = 0;

        bool wait_for_signal = true;
        size_t identical_sample_num = 0;

        size_t nb_success = PacketSamples * SourcePackets * 4;

        float prev_sample = sample_step_;

        while (identical_sample_num < nb_success) {
            size_t i = 0;
            frame_num++;

            roc_frame frame;
            memset(&frame, 0, sizeof(frame));

            frame.samples = rx_buff;
            frame.samples_size = frame_size_ * sizeof(float);

            roc_panic_if_not(roc_receiver_read(recv_, &frame) == 0);

            if (wait_for_signal) {
                for (; i < frame_size_ && is_zero_(rx_buff[i]); i++) {
                }

                if (i < frame_size_) {
                    wait_for_signal = false;

                    prev_sample = rx_buff[i];
                    i++;
                }
            }

            if (!wait_for_signal) {
                float cur_rx_buff;
                for (; i < frame_size_; i++, sample_num++) {
                    cur_rx_buff = rx_buff[i];

                    if (is_zero_(increment_sample_value(prev_sample, sample_step_)
                                 - cur_rx_buff)) {
                        identical_sample_num++;
                    } else if (!is_zero_(prev_sample)
                               && !is_zero_(cur_rx_buff)) { // Allows stream shifts
                        char sbuff[256];
                        int sbuff_i =
                            snprintf(sbuff, sizeof(sbuff),
                                     "failed comparing sample #%lu\n\nframe_num: %lu\n",
                                     (unsigned long)identical_sample_num,
                                     (unsigned long)frame_num);
                        snprintf(
                            &sbuff[sbuff_i], sizeof(sbuff) - (size_t)sbuff_i,
                            "original: %f,\treceived: %f\n",
                            (double)increment_sample_value(prev_sample, sample_step_),
                            (double)cur_rx_buff);
                        roc_panic("%s", sbuff);
                    }

                    prev_sample = cur_rx_buff;
                }
            }
        }
    }

private:
    static inline bool is_zero_(float s) {
        return fabs(double(s)) < 1e-9;
    }

    roc_receiver* recv_;

    roc_endpoint* source_endp_;
    roc_endpoint* repair_endp_;

    const float sample_step_;
    const size_t frame_size_;
};

} // namespace test
} // namespace library
} // namespace roc

#endif // ROC_LIBRARY_TEST_HELPERS_RECEIVER_H_
