/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/node.h"

namespace roc {
namespace node {

Node::Node(Context& context)
    : core::ArenaAllocation(context.arena())
    , context_(&context) {
}

Node::~Node() {
}

Context& Node::context() {
    return *context_;
}

} // namespace node
} // namespace roc
