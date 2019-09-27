/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/ibackend.h
//! @brief Backend interface.

#ifndef ROC_SNDIO_IBACKEND_H_
#define ROC_SNDIO_IBACKEND_H_

#include "roc_core/iallocator.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_list.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Backend interface.
class IBackend {
public:
    virtual ~IBackend();

    //! Probing flags.
    enum FilterFlags {
        //! Input or output may be a sink.
        FilterSink = (1 << 0),

        //! Input or output may be a source.
        FilterSource = (1 << 1),

        //! Input or output may be a file.
        FilterFile = (1 << 2),

        //! Input or output may be a device.
        FilterDevice = (1 << 3)
    };

    //! Check whether the backend can handle given input or output.
    virtual bool probe(const char* driver, const char* inout, int filter_flags) = 0;

    //! Create and open a sink.
    virtual ISink* open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config) = 0;

    //! Create and open a source.
    virtual ISource* open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config) = 0;

    //! Append supported drivers to the list.
    virtual bool get_drivers(core::StringList&, int filter_flags) = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IBACKEND_H_
