/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/stream.h
//! @brief Stream of bytes.

#ifndef ROC_NETIO_STREAM_H_
#define ROC_NETIO_STREAM_H_

#include "roc_core/list.h"
#include "roc_core/mutex.h"
#include "roc_netio/stream_buffer.h"

namespace roc {
namespace netio {

//! Thread-safe stream of bytes.
class Stream {
public:
    //! Destroy.
    ~Stream();

    //! Return number of bytes in stream available to read.
    size_t size() const;

    //! Append @p buffer to the stream.
    void append(const StreamBufferPtr& buffer);

    //! Read @p len bytes to @p data from the stream.
    //!
    //! @remarks
    //!  - @p buf should not be null.
    //!  - @p buf should have size at least of @p len bytes.
    //!
    //! @returns
    //!  the number of bytes read or -1 if some error occurred.
    ssize_t read(char* buf, size_t len);

private:
    core::Mutex mutex_;
    core::List<StreamBuffer> buffers_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_STREAM_H_
