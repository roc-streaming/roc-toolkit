/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/target_stdio/roc_pipeline/port_to_str.h
//! @brief Port to string.

#ifndef ROC_PIPELINE_PORT_TO_STR_H_
#define ROC_PIPELINE_PORT_TO_STR_H_

#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Convert pipeline port to string.
class port_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit port_to_str(const PortConfig&);

    //! Get formatted address.
    const char* c_str() const {
        return buffer_;
    }

private:
    char buffer_[256];
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PORT_TO_STR_H_
