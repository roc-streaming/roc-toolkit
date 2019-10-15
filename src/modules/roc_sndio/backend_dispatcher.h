/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/backend_dispatcher.h
//! @brief Backend dispatcher.

#ifndef ROC_SNDIO_BACKEND_DISPATCHER_H_
#define ROC_SNDIO_BACKEND_DISPATCHER_H_

#include "roc_address/io_uri.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/singleton.h"
#include "roc_core/string_list.h"
#include "roc_sndio/ibackend.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Backend dispatcher.
class BackendDispatcher : public core::NonCopyable<> {
public:
    //! Get instance.
    static BackendDispatcher& instance() {
        return core::Singleton<BackendDispatcher>::instance();
    }

    //! Set internal buffer size for all backends that need it.
    void set_frame_size(core::nanoseconds_t frame_length,
                        size_t sample_rate,
                        packet::channel_mask_t channels);

    //! Create and open a sink.
    ISink* open_sink(core::IAllocator& allocator,
                     const address::IoURI& uri,
                     const char* force_format,
                     const Config& config);

    //! Create and open a source.
    ISource* open_source(core::IAllocator& allocator,
                         const address::IoURI& uri,
                         const char* force_format,
                         const Config& config);

    //! Get all supported URI schemes.
    bool get_supported_schemes(core::StringList&);

    //! Get all supported file formats.
    bool get_supported_formats(core::StringList&);

private:
    friend class core::Singleton<BackendDispatcher>;

    BackendDispatcher();

    IBackend* find_backend_(const char* driver, const char* inout, int flags);
    void register_backend_(IBackend& backend);

    enum { MaxBackends = 8 };

    IBackend* backends_[MaxBackends];
    size_t n_backends_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_BACKEND_DISPATCHER_H_
