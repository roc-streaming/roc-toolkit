/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/ibasic_session_composer.h
//! @brief BasicSession composer interface.

#ifndef ROC_PIPELINE_IBASIC_SESSION_COMPOSER_H_
#define ROC_PIPELINE_IBASIC_SESSION_COMPOSER_H_

#include "roc_pipeline/basic_session.h"

namespace roc {
namespace pipeline {

//! BasicSession composer interface.
class IBasicSessionComposer {
public:
    virtual ~IBasicSessionComposer();

    //! Create new session.
    virtual BasicSessionPtr compose() = 0;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_IBASIC_SESSION_COMPOSER_H_
