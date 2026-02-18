/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/node.h
//! @brief Base class for nodes.

#ifndef ROC_NODE_NODE_H_
#define ROC_NODE_NODE_H_

#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_node/context.h"

namespace roc {
namespace node {

//! Base class for nodes.
class Node : public core::ArenaAllocation, core::NonCopyable<> {
public:
    //! Initialize.
    explicit Node(Context& context);

    //! Deinitialize.
    virtual ~Node();

    //! All nodes hold reference to context.
    Context& context();

private:
    core::SharedPtr<Context> context_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_NODE_H_
