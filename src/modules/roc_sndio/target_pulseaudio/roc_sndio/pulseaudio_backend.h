/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_pulseaudio/roc_sndio/pulseaudio_backend.h
//! @brief Pulseaudio backend.

#ifndef ROC_SNDIO_PULSEAUDIO_BACKEND_H_
#define ROC_SNDIO_PULSEAUDIO_BACKEND_H_

#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! Pulseaudio backend.
class PulseaudioBackend : public IBackend, core::NonCopyable<> {
public:
    //! Get instance.
    static PulseaudioBackend& instance() {
        return core::Singleton<PulseaudioBackend>::instance();
    }

    //! Create and open a sink.
    virtual ISink* open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config,
                             int filter_flags);

    //! Create and open a source.
    virtual ISource* open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config,
                                 int filter_flags);

    //! Append supported drivers to the list.
    virtual bool get_drivers(core::Array<DriverInfo>&, int filter_flags);

private:
    friend class core::Singleton<PulseaudioBackend>;

    PulseaudioBackend();
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PULSEAUDIO_BACKEND_H_
