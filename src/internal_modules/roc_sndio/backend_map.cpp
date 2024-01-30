/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/backend_map.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

BackendMap::BackendMap() {
    register_backends_();
    register_drivers_();

    roc_log(LogDebug, "backend map: initializing: n_backends=%d n_drivers=%d",
            (int)backends_.size(), (int)drivers_.size());
}

size_t BackendMap::num_backends() const {
    return backends_.size();
}

IBackend& BackendMap::nth_backend(size_t backend_index) const {
    return *backends_[backend_index];
}

size_t BackendMap::num_drivers() const {
    return drivers_.size();
}

const DriverInfo& BackendMap::nth_driver(size_t driver_index) const {
    return drivers_[driver_index];
}

void BackendMap::set_frame_size(core::nanoseconds_t frame_length,
                                const audio::SampleSpec& sample_spec) {
#ifdef ROC_TARGET_SOX
    sox_backend_->set_frame_size(frame_length, sample_spec);
#endif // ROC_TARGET_SOX
    (void)frame_length;
    (void)sample_spec;
}

void BackendMap::register_backends_() {
    wav_backend_.reset(new (wav_backend_) WavBackend);
    add_backend_(wav_backend_.get());

#ifdef ROC_TARGET_PULSEAUDIO
    pulseaudio_backend_.reset(new (pulseaudio_backend_) PulseaudioBackend);
    add_backend_(pulseaudio_backend_.get());
#endif // ROC_TARGET_PULSEAUDIO

#ifdef ROC_TARGET_SOX
    sox_backend_.reset(new (sox_backend_) SoxBackend);
    add_backend_(sox_backend_.get());
#endif // ROC_TARGET_SOX
}

void BackendMap::register_drivers_() {
    for (size_t n = 0; n < backends_.size(); n++) {
        backends_[n]->discover_drivers(drivers_);
    }
}

void BackendMap::add_backend_(IBackend* backend) {
    if (!backends_.push_back(backend)) {
        roc_panic("backend map: can't register backend");
    }
}

} // namespace sndio
} // namespace roc
