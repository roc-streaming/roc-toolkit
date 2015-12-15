/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_datagram/idatagram_writer.h
//! @brief Datagram writer interface.

#ifndef ROC_DATAGRAM_IDATAGRAM_WRITER_H_
#define ROC_DATAGRAM_IDATAGRAM_WRITER_H_

#include "roc_datagram/idatagram.h"

namespace roc {
namespace datagram {

//! Datagram writer interface.
class IDatagramWriter {
public:
    virtual ~IDatagramWriter();

    //! Write datagram.
    virtual void write(const IDatagramPtr&) = 0;
};

} // namespace datagram
} // namespace roc

#endif // ROC_DATAGRAM_IDATAGRAM_WRITER_H_
