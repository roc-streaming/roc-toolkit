/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_LIBRARY_TEST_HELPERS_SENDER_H_
#define ROC_LIBRARY_TEST_HELPERS_SENDER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/context.h"
#include "test_helpers/utils.h"

#include "roc_core/panic.h"
#include "roc_core/thread.h"

#include "roc/context.h"
#include "roc/endpoint.h"
#include "roc/sender.h"

namespace roc {
namespace library {
namespace test {

class Sender : public core::Thread {
public:
    Sender(Context& context,
           roc_sender_config& config,
           const roc_endpoint* receiver_source_endp,
           const roc_endpoint* receiver_repair_endp,
           float sample_step,
           size_t frame_size,
           unsigned flags)
        : sndr_(NULL)
        , sample_step_(sample_step)
        , frame_size_(frame_size)
        , stopped_(false) {
        CHECK(roc_sender_open(context.get(), &config, &sndr_) == 0);
        CHECK(sndr_);

        if (flags & FlagRS8M || flags & FlagLDPC) {
            CHECK(roc_sender_connect(sndr_, ROC_INTERFACE_AUDIO_SOURCE,
                                     receiver_source_endp)
                  == 0);
            CHECK(roc_sender_connect(sndr_, ROC_INTERFACE_AUDIO_REPAIR,
                                     receiver_repair_endp)
                  == 0);
        } else {
            CHECK(roc_sender_connect(sndr_, ROC_INTERFACE_AUDIO_SOURCE,
                                     receiver_source_endp)
                  == 0);
        }
    }

    ~Sender() {
        CHECK(roc_sender_close(sndr_) == 0);
    }

    void stop() {
        stopped_ = true;
    }

private:
    virtual void run() {
        float sample_value = sample_step_;
        float samples[TotalSamples];

        while (!stopped_) {
            for (size_t i = 0; i < TotalSamples; ++i) {
                samples[i] = sample_value;
                sample_value = increment_sample_value(sample_value, sample_step_);
            }

            for (size_t off = 0; off < TotalSamples; off += frame_size_) {
                if (off + frame_size_ > TotalSamples) {
                    off = TotalSamples - frame_size_;
                }

                roc_frame frame;
                memset(&frame, 0, sizeof(frame));

                frame.samples = samples + off;
                frame.samples_size = frame_size_ * sizeof(float);

                const int ret = roc_sender_write(sndr_, &frame);
                roc_panic_if_not(ret == 0);
            }
        }
    }

    roc_sender* sndr_;
    const float sample_step_;
    const size_t frame_size_;
    core::Atomic<int> stopped_;
};

} // namespace test
} // namespace library
} // namespace roc

#endif // ROC_LIBRARY_TEST_HELPERS_SENDER_H_
