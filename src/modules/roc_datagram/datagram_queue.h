/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_datagram/datagram_queue.h
//! @brief Datagram queue.

#ifndef ROC_DATAGRAM_DATAGRAM_QUEUE_H_
#define ROC_DATAGRAM_DATAGRAM_QUEUE_H_

#include "roc_config/config.h"

#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/mutex.h"

#include "roc_datagram/idatagram_reader.h"
#include "roc_datagram/idatagram_writer.h"

namespace roc {
namespace datagram {

//! Datagram queue.
class DatagramQueue : public IDatagramReader,
                      public IDatagramWriter,
                      public core::NonCopyable<> {
public:
    //! Construct empty queue.
    //!
    //! If @p max_size is non-zero, it defines maximum number of
    //! datagrams in the queue. If maximum size is reached when
    //! datagram is added, the oldest datagram is dropped.
    DatagramQueue(size_t max_size = ROC_CONFIG_MAX_DATAGRAMS);

    //! Read datagram.
    virtual IDatagramPtr read();

    //! Write datagram.
    virtual void write(const IDatagramPtr&);

    //! Number of datagrams in queue.
    size_t size() const;

private:
    typedef core::Mutex::Lock Lock;

    core::Mutex mutex_;
    core::List<IDatagram> list_;

    const size_t max_size_;
};

} // namespace datagram
} // namespace roc

#endif // ROC_DATAGRAM_DATAGRAM_QUEUE_H_
