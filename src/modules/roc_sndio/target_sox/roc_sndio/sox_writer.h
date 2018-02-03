/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_writer.h
//! @brief SoX audio writer.

#ifndef ROC_SNDIO_SOX_WRITER_H_
#define ROC_SNDIO_SOX_WRITER_H_

#include <sox.h>

#include "roc_audio/iwriter.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/units.h"

namespace roc {
namespace sndio {

//! SoX audio writer.
//! @remarks
//!  Encodes samples them and and writes to output file or audio driver.
class SoxWriter : public audio::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p channels defines bitmask of enabled channels in input buffers
    //!  - @p sample_rate defines sample rate of input buffers
    SoxWriter(core::IAllocator& allocator,
              packet::channel_mask_t channels,
              size_t sample_rate);

    virtual ~SoxWriter();

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
    bool open(const char* name, const char* type);

    //! Get sample rate of an output file or a device.
    //!
    //! @pre
    //!  Output file or device should be opened.
    size_t sample_rate() const;

    //! Returns true if output is a real file.
    //!
    //! @pre
    //!  Output file or device should be opened.
    bool is_file() const;

    //! Returns recommended frame size.
    size_t frame_size() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    bool prepare_();
    bool open_(const char* name, const char* type);
    void write_(const sox_sample_t* samples, size_t n_samples);
    void close_();

    sox_format_t* output_;
    sox_signalinfo_t out_signal_;

    core::IAllocator& allocator_;

    core::UniquePtr<sox_sample_t> buffer_;
    size_t buffer_size_;

    bool is_file_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_WRITER_H_
