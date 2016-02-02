/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/reader.h
//! @brief Audio reader.

#ifndef ROC_SNDIO_READER_H_
#define ROC_SNDIO_READER_H_

#include <sox.h>

#include "roc_config/config.h"

#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_core/atomic.h"

#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace sndio {

//! Audio reader.
//! @remarks
//!  Reads samples from input file or audio driver, decodes them and
//!  writes buffers to output writer.
class Reader : public core::Thread {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p output is used to write buffers with decoded samples;
    //!  - @p composer is used to allocate buffers;
    //!  - @p n_samples defines number of samples per channel in output buffers;
    //!  - @p channels defines bitmask of enabled channels in output buffers;
    //!  - @p sample_rate defines sample rate of output buffers.
    Reader(audio::ISampleBufferWriter& output,
           audio::ISampleBufferComposer& composer = audio::default_buffer_composer(),
           packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
           size_t n_samples = ROC_CONFIG_DEFAULT_RECEIVER_TICK_SAMPLES,
           size_t sample_rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE);

    ~Reader();

    //! Open input file or device.
    //!
    //! @b Parameters
    //!  - @p name is input file or device name, "-" for stdin.
    //!  - @p type is codec or driver name.
    //!
    //! @remarks
    //!  If @p name or @p type are NULL, they're autodetected.
    //!
    //! @pre
    //!  Should be called once before calling start().
    bool open(const char* name = NULL, const char* type = NULL);

    //! Stop thread.
    //! @remarks
    //!  Can be called from any thread.
    void stop();

private:
    virtual void run();

    static const sox_effect_handler_t output_handler_;

    static int kill_cb_(sox_effect_t* eff);
    static int output_cb_(sox_effect_t* eff,
                          const sox_sample_t* ibuf,
                          sox_sample_t* obuf,
                          size_t* ibufsz,
                          size_t* obufsz);

    void write_(const sox_sample_t* buf, size_t bufsz, bool eof);
    void close_();

    sox_signalinfo_t out_signal_;

    sox_format_t* input_;
    sox_effects_chain_t* chain_;

    audio::ISampleBufferWriter& output_;
    audio::ISampleBufferComposer& composer_;

    audio::ISampleBufferPtr buffer_;
    size_t buffer_pos_;

    size_t buffer_size_;
    size_t clips_;
    size_t n_bufs_;

    core::Atomic stop_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_READER_H_
