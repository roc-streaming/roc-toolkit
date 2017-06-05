/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/byte_order.h
//! @brief Endian converters.

#ifndef ROC_CORE_BYTE_ORDER_H_
#define ROC_CORE_BYTE_ORDER_H_

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/types.h>

//! Network to host (16 bits).
#define ROC_NTOH_16(v) ntohs(v)

//! Network to host (32 bits).
#define ROC_NTOH_32(v) ntohl(v)

//! Host to network (16 bits).
#define ROC_HTON_16(v) htons(v)

//! Host to network (32 bits).
#define ROC_HTON_32(v) htonl(v)

#endif // ROC_CORE_BYTE_ORDER_H_
