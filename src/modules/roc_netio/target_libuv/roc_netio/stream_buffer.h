/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/stream_buffer.h
//! @brief Dynamic stream of buffers.

#ifndef ROC_NETIO_STREAM_BUFFER_H_
#define ROC_NETIO_STREAM_BUFFER_H_

#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

//! Bytes buffer.
class StreamBuffer : public core::RefCnt<StreamBuffer>, public core::ListNode {
public:
    //! Initialize empty buffer;
    explicit StreamBuffer(core::IAllocator& allocator);

    //! Get number of bytes in buffer.
    size_t size() const;

    //! Returns the underlying buffer data.
    char* data();

    //! Resize the buffer to the desired size.
    bool resize(size_t);

    //! Read @p len bytes to @p data from the buffer.
    //!
    //! @remarks
    //!  - @p buf should not be null.
    //!  - @p buf should have size at least of @p len bytes.
    //!
    //! @returns
    //!  the number of bytes read or -1 if some error occurred.
    ssize_t read(char* buf, size_t len);

private:
    friend class core::RefCnt<StreamBuffer>;

    void destroy();

    core::IAllocator& allocator_;

    core::Array<char> data_;

    size_t offset_;
};

//! Stream buffer smart pointer.
typedef core::SharedPtr<StreamBuffer> StreamBufferPtr;

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_STREAM_BUFFER_H_
