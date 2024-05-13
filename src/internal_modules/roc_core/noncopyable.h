/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/noncopyable.h
//! @brief Non-copyable object.

#ifndef ROC_CORE_NONCOPYABLE_H_
#define ROC_CORE_NONCOPYABLE_H_

namespace roc {
namespace core {

//! Base class for non-copyable objects.
//! @note
//!  Template allows instantiate distinct non-copyable bases when
//!  they are inherited by multiple paths. We need this to
//!  eliminate compiler warnings "inaccessible direct base...".
template <class Tag = void> class NonCopyable {
protected:
    NonCopyable() {
    }

private:
    NonCopyable(const NonCopyable&);
    NonCopyable& operator=(const NonCopyable&);
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_NONCOPYABLE_H_
