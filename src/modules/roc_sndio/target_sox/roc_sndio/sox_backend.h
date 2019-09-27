/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_backend.h
//! @brief SoX backend.

#ifndef ROC_SNDIO_SOX_BACKEND_H_
#define ROC_SNDIO_SOX_BACKEND_H_

#include <sox.h>

#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! SoX backend.
class SoxBackend : public IBackend, core::NonCopyable<> {
public:
    //! Get instance.
    static SoxBackend& instance() {
        return core::Singleton<SoxBackend>::instance();
    }

    //! Set internal SoX frame size.
    //! @remarks
    //!  Number of samples for all channels.
    void set_frame_size(size_t size);

    //! Check whether the backend can handle given input or output.
    virtual bool probe(const char* driver, const char* inout, int flags);

    //! Create and open a sink.
    virtual ISink* open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config);

    //! Create and open a source.
    virtual ISource* open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config);

    //! Append supported drivers to the list.
    virtual bool get_drivers(core::StringList&, int filter_flags);

private:
    friend class core::Singleton<SoxBackend>;

    SoxBackend();

    core::Mutex mutex_;

    bool first_created_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_BACKEND_H_
