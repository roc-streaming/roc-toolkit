/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/wav_source.h
//! @brief WAV source.

#ifndef ROC_SNDIO_WAV_SOURCE_H_
#define ROC_SNDIO_WAV_SOURCE_H_

#include <dr_wav.h>

#include "roc_audio/frame_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/string_buffer.h"
#include "roc_sndio/io_config.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! WAV source.
//! @remarks
//!  Reads samples from input WAV file.
class WavSource : public ISource, private core::NonCopyable<> {
public:
    //! Initialize.
    WavSource(audio::FrameFactory& frame_factory,
              core::IArena& arena,
              const IoConfig& io_config,
              const char* path);
    ~WavSource();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get device type.
    virtual DeviceType type() const;

    //! Try to cast to ISink.
    virtual ISink* to_sink();

    //! Try to cast to ISource.
    virtual ISource* to_source();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get recommended frame length of the source.
    virtual core::nanoseconds_t frame_length() const;

    //! Check if the source supports state updates.
    virtual bool has_state() const;

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Restart reading from beginning.
    virtual ROC_ATTR_NODISCARD status::StatusCode rewind();

    //! Adjust source clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t timestamp);

    //! Read frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(audio::Frame& frame,
         packet::stream_timestamp_t duration,
         audio::FrameReadMode mode);

    //! Explicitly close the source.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Destroy object and return memory to arena.
    virtual void dispose();

private:
    status::StatusCode open_(const char* path);
    status::StatusCode close_();

    audio::FrameFactory& frame_factory_;

    audio::SampleSpec sample_spec_;

    FILE* input_file_;
    drwav wav_decoder_;
    bool eof_;

    status::StatusCode init_status_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WAV_SOURCE_H_
