/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/writer.h
//! @brief Audio writer.

#ifndef ROC_SNDIO_WRITER_H_
#define ROC_SNDIO_WRITER_H_

#include <sox.h>

#include "roc_config/config.h"

#include "roc_core/atomic.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"

#include "roc_audio/isample_buffer_reader.h"

namespace roc {
namespace sndio {

//! Audio writer.
//! @remarks
//!  Reads samples in interleaved format, encodes them and and writes to
//!  output file or audio driver.
class Writer : public core::Thread {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p input is used to read samples;
    //!  - @p channels defines bitmask of enabled channels in input buffers;
    //!  - @p sample_rate defines sample rate of input buffers.
    Writer(audio::ISampleBufferReader& input,
           packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
           size_t sample_rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE);

    ~Writer();

    //! Open output file or device.
    //!
    //! @b Parameters
    //!  - @p name is output file or device name, "-" for stdout.
    //!  - @p type is codec or driver name.
    //!
    //! @remarks
    //!  If @p name or @p type are NULL, they're autodetected.
    //!
    //! @pre
    //!  Should be called once before calling start().
    bool open(const char* name, const char* type = NULL);

    //! Stop thread.
    //! @remarks
    //!  Can be called from any thread.
    void stop();

private:
    virtual void run();

    void loop_();

    bool write_(const sox_sample_t* samples, size_t n_samples);
    void close_();

    sox_format_t* output_;
    sox_signalinfo_t out_signal_;

    audio::ISampleBufferReader& input_;

    size_t clips_;
    size_t n_bufs_;

    core::Atomic stop_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WRITER_H_
