/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/list_node.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

ListNode::~ListNode() {
    if (node_.list != NULL) {
        roc_panic("attempting to call destructor for element that is still in list");
    }
}

} // namespace core
} // namespace roc
