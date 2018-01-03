/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_uv/roc_core/trigger.h
//! @brief Trigger.

#ifndef ROC_CORE_TRIGGER_H_
#define ROC_CORE_TRIGGER_H_

#include <uv.h>

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Trigger.
class Trigger : public NonCopyable<> {
public:
    //! Initialize trigger with the given initial value.
    explicit Trigger(bool state);

    ~Trigger();

    //! Set trigger state.
    //! @remarks
    //!  If state becomes true, notifies blocked wait() calls.
    void set(bool state);

    //! Get trigger state.
    //! @remarks
    //!  It's not guaranteed that the state is still true when get()
    //!  returns if there are other threads that may call set().
    bool get() const;

    //! Wait until trigger state becomes true.
    //! @remarks
    //!  It's not guaranteed that the state is still true when wait()
    //!  returns if there are other threads that may call set().
    void wait() const;

private:
    mutable uv_mutex_t mutex_;
    mutable uv_cond_t cond_;
    bool state_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_TRIGGER_H_
