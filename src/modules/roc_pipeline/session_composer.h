/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/session_composer.h
//! @brief Session composer.

#ifndef ROC_PIPELINE_SESSION_COMPOSER_H_
#define ROC_PIPELINE_SESSION_COMPOSER_H_

#include "roc_core/singleton.h"

#include "roc_pipeline/ibasic_session_composer.h"
#include "roc_pipeline/session.h"

namespace roc {
namespace pipeline {

//! Session composer.
class SessionComposer : public IBasicSessionComposer {
public:
    virtual ~SessionComposer();

    //! Initialize.
    SessionComposer(core::IPool<Session>& pool = core::HeapPool<Session>::instance())
        : pool_(pool) {
    }

    //! Create new session.
    virtual BasicSessionPtr compose() {
        return new (pool_) Session(pool_);
    }

private:
    core::IPool<Session>& pool_;
};

//! Get default session composer.
static inline IBasicSessionComposer& default_session_composer() {
    return core::Singleton<SessionComposer>::instance();
}

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SESSION_COMPOSER_H_
