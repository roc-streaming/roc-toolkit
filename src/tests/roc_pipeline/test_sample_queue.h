/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_SAMPLE_QUEUE_H_
#define ROC_PIPELINE_TEST_SAMPLE_QUEUE_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/circular_buffer.h"
#include "roc_core/noncopyable.h"

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace test {

template <size_t MaxSz>
class SampleQueue : public audio::ISampleBufferReader,
                    public audio::ISampleBufferWriter,
                    public core::NonCopyable<> {
public:
    virtual audio::ISampleBufferConstSlice read() {
        CHECK(queue_.size() != 0);

        return queue_.shift();
    }

    virtual void write(const audio::ISampleBufferConstSlice& buffer) {
        CHECK(queue_.size() < queue_.max_size());

        queue_.push(buffer);
    }

    size_t size() const {
        return queue_.size();
    }

    void clear() {
        queue_.clear();
    }

private:
    core::CircularBuffer<audio::ISampleBufferConstSlice, MaxSz> queue_;
};

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_SAMPLE_QUEUE_H_
