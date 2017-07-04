/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/semaphore.h"

namespace roc {
namespace core {

Semaphore::Semaphore(size_t counter)
    : counter_(counter) {
    if (int err = uv_mutex_init(&mutex_)) {
        roc_panic("uv_mutex_init(): [%s] %s", uv_err_name(err), uv_strerror(err));
    }
    if (int err = uv_cond_init(&cond_)) {
        roc_panic("uv_cond_init(): [%s] %s", uv_err_name(err), uv_strerror(err));
    }
}

Semaphore::~Semaphore() {
    uv_mutex_destroy(&mutex_);
    uv_cond_destroy(&cond_);
}

void Semaphore::post() {
    uv_mutex_lock(&mutex_);

    counter_++;

    uv_mutex_unlock(&mutex_);

    uv_cond_broadcast(&cond_);
}

void Semaphore::pend() {
    uv_mutex_lock(&mutex_);

    while (counter_ == 0) {
        uv_cond_wait(&cond_, &mutex_);
    }

    counter_--;

    uv_mutex_unlock(&mutex_);
}

bool Semaphore::try_pend() {
    bool ret = false;

    uv_mutex_lock(&mutex_);

    if (counter_ != 0) {
        counter_--;
        ret = true;
    }

    uv_mutex_unlock(&mutex_);

    return ret;
}

void Semaphore::wait() {
    uv_mutex_lock(&mutex_);

    while (counter_ == 0) {
        uv_cond_wait(&cond_, &mutex_);
    }

    uv_mutex_unlock(&mutex_);
}

} // namespace core
} // namespace roc
