/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/backend_map.h"
#include "roc_core/noop_arena.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

BackendMap::BackendMap()
    : backends_(core::NoopArena)
    , drivers_(core::NoopArena)
    , formats_(core::NoopArena) {
    register_backends_();
    collect_drivers_();
    collect_formats_();

    roc_log(LogDebug,
            "backend map: initializing: n_backends=%d n_drivers=%d n_formats=%d",
            (int)backends_.size(), (int)drivers_.size(), (int)formats_.size());
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

size_t BackendMap::num_formats() const {
    return formats_.size();
}

const FormatInfo& BackendMap::nth_format(size_t format_index) const {
    return formats_[format_index];
}

void BackendMap::register_backends_() {
#ifdef ROC_TARGET_PULSEAUDIO
    pulseaudio_backend_.reset(new (pulseaudio_backend_) PulseaudioBackend);
    add_backend_(pulseaudio_backend_.get());
#endif // ROC_TARGET_PULSEAUDIO

#ifdef ROC_TARGET_SNDFILE
    sndfile_backend_.reset(new (sndfile_backend_) SndfileBackend);
    add_backend_(sndfile_backend_.get());
#endif // ROC_TARGET_SNDFILE

    wav_backend_.reset(new (wav_backend_) WavBackend);
    add_backend_(wav_backend_.get());

#ifdef ROC_TARGET_SOX
    sox_backend_.reset(new (sox_backend_) SoxBackend);
    add_backend_(sox_backend_.get());
#endif // ROC_TARGET_SOX
}

void BackendMap::add_backend_(IBackend* backend) {
    if (!backends_.push_back(backend)) {
        roc_panic("backend map: can't register backend");
    }
}

void BackendMap::collect_drivers_() {
    for (size_t n = 0; n < backends_.size(); n++) {
        if (!backends_[n]->discover_drivers(drivers_)) {
            roc_panic("backend map: can't register driver");
        }
    }
}

void BackendMap::collect_formats_() {
    for (size_t n = 0; n < backends_.size(); n++) {
        if (!backends_[n]->discover_formats(formats_)) {
            roc_panic("backend map: can't register format");
        }
    }
}

} // namespace sndio
} // namespace roc
