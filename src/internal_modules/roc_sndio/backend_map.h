/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/backend_map.h
//! @brief Backend map.

#ifndef ROC_SNDIO_BACKEND_MAP_H_
#define ROC_SNDIO_BACKEND_MAP_H_

#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/singleton.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/ibackend.h"

#ifdef ROC_TARGET_PULSEAUDIO
#include "roc_sndio/pulseaudio_backend.h"
#endif // ROC_TARGET_PULSEAUDIO

#ifdef ROC_TARGET_SOX
#include "roc_sndio/sox_backend.h"
#endif // ROC_TARGET_SOX

namespace roc {
namespace sndio {

//! Backend map.
class BackendMap : public core::NonCopyable<> {
public:
    //! Get instance.
    static BackendMap& instance() {
        return core::Singleton<BackendMap>::instance();
    }

    //! Get number of backends available.
    size_t num_backends() const;

    //! Get backend by index.
    IBackend& nth_backend(size_t backend_index) const;

    //! Get number of drivers available.
    size_t num_drivers() const;

    //! Get driver by index.
    const DriverInfo& nth_driver(size_t driver_index) const;

    //! Set internal buffer size for all backends that need it.
    void set_frame_size(core::nanoseconds_t frame_length,
                        const audio::SampleSpec& sample_spec);

private:
    friend class core::Singleton<BackendMap>;

    BackendMap();

    void register_backends_();
    void register_drivers_();

    void add_backend_(IBackend*);

#ifdef ROC_TARGET_PULSEAUDIO
    core::Optional<PulseaudioBackend> pulseaudio_backend_;
#endif // ROC_TARGET_PULSEAUDIO

#ifdef ROC_TARGET_SOX
    core::Optional<SoxBackend> sox_backend_;
#endif // ROC_TARGET_SOX

    core::Array<IBackend*, MaxBackends> backends_;
    core::Array<DriverInfo, MaxDrivers> drivers_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_BACKEND_MAP_H_
