/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/basic_client.h
//! @brief Base class for client pipeline.

#ifndef ROC_PIPELINE_BASIC_CLIENT_H_
#define ROC_PIPELINE_BASIC_CLIENT_H_

#include "roc_core/noncopyable.h"
#include "roc_core/thread.h"

#include "roc_datagram/idatagram_writer.h"

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"

#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Base class for client pipeline.
//!
//! @remarks
//!  Fetches samples from input reader and sends them to output writer.
//!
//! @see Client.
class BasicClient : public core::Thread, public core::NonCopyable<> {
public:
    //! Process input samples.
    //! @remarks
    //!  Fetches one sample buffer from input reader.
    bool tick();

protected:
    //! Initialize client.
    BasicClient(const ClientConfig& config, datagram::IDatagramWriter& datagram_writer);

    //! Run client thread.
    virtual void run();

    //! Create input audio reader.
    virtual audio::ISampleBufferReader* make_audio_reader() = 0;

    //! Create output audio reader.
    virtual audio::ISampleBufferWriter* make_audio_writer() = 0;

    //! Get config.
    const ClientConfig& config() const;

private:
    const ClientConfig config_;

    audio::ISampleBufferReader* audio_reader_;
    audio::ISampleBufferWriter* audio_writer_;

    datagram::IDatagramWriter& datagram_writer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_BASIC_CLIENT_H_
