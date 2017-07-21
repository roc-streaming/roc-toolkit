/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_updater.h
//! @brief Resampler updater.

#ifndef ROC_AUDIO_RESAMPLER_UPDATER_H_
#define ROC_AUDIO_RESAMPLER_UPDATER_H_

#include "roc_audio/freq_estimator.h"
#include "roc_audio/resampler.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Updates Resampler scaling factor using FreqEstimator.
class ResamplerUpdater : public packet::IWriter,
                         public packet::IReader,
                         public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p update_interval defines how often to call FreqEstimator, in samples
    //!  - @p aim_queue_size defines FreqEstimator target queue size, in samples
    ResamplerUpdater(packet::timestamp_t update_interval,
                     packet::timestamp_t aim_queue_size);

    //! Set output writer.
    void set_writer(packet::IWriter&);

    //! Set input reader.
    void set_reader(packet::IReader&);

    //! Set resampler.
    void set_resampler(Resampler&);

    //! Write packet.
    virtual void write(const packet::PacketPtr&);

    //! Read packet.
    virtual packet::PacketPtr read();

    //! Update resampler.
    //! @returns
    //!  false if the calculated freq coeff gone beyond the boundaries.
    bool update(packet::timestamp_t time);

private:
    packet::IWriter* writer_;
    packet::IReader* reader_;

    Resampler* resampler_;
    FreqEstimator fe_;

    core::RateLimiter rate_limiter_;

    const packet::timestamp_t update_interval_;
    packet::timestamp_t update_time_;
    packet::timestamp_t start_time_;

    bool has_first_;
    packet::timestamp_t first_;

    bool has_last_;
    packet::timestamp_t last_;

    bool started_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_UPDATER_H_
