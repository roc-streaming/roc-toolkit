/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/freelist.h
//! @brief TODO.

#ifndef ROC_CORE_FREELIST_H_
#define ROC_CORE_FREELIST_H_

namespace roc {
namespace core {

template <class T, class Node = ListNode<> > : public NonCopyable<> {
public:
    FreeList()
        : member_initialization {
    }
}
} // namespace core

#endif // ROC_CORE_FREELIST_H_
