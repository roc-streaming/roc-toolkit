/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/trigger.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

Trigger::Trigger(bool state)
    : state_(state) {
    if (int err = uv_mutex_init(&mutex_)) {
        roc_panic("trigger: uv_mutex_init(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
    if (int err = uv_cond_init(&cond_)) {
        roc_panic("trigger: uv_cond_init(): [%s] %s", uv_err_name(err), uv_strerror(err));
    }
}

Trigger::~Trigger() {
    uv_mutex_destroy(&mutex_);
    uv_cond_destroy(&cond_);
}

void Trigger::set(bool state) {
    uv_mutex_lock(&mutex_);

    state_ = state;

    uv_mutex_unlock(&mutex_);

    if (state) {
        uv_cond_broadcast(&cond_);
    }
}

bool Trigger::get() const {
    uv_mutex_lock(&mutex_);

    const bool state = state_;

    uv_mutex_unlock(&mutex_);

    return state;
}

void Trigger::wait() const {
    uv_mutex_lock(&mutex_);

    while (!state_) {
        uv_cond_wait(&cond_, &mutex_);
    }

    uv_mutex_unlock(&mutex_);
}

} // namespace core
} // namespace roc
