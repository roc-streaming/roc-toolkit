/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/global_destructor.h
//! @brief Global destructor.

#ifndef ROC_CORE_GLOBAL_DESTRUCTOR_H_
#define ROC_CORE_GLOBAL_DESTRUCTOR_H_

namespace roc {
namespace core {

//! Allows to determine if global library destruction was initiated.
class GlobalDestructor {
public:
    ~GlobalDestructor();

    //! Check if global library destruction was initiated.
    static bool is_destroying();
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_GLOBAL_DESTRUCTOR_H_
