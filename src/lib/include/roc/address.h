/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc/address.h
//! @brief Roc network address.

#ifndef ROC_ADDRESS_H_
#define ROC_ADDRESS_H_

#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Network address family.
typedef enum roc_family {
    //! Invalid address.
    ROC_AF_INVALID = -1,

    //! Automatically detect address family from address format.
    ROC_AF_AUTO = 0,

    //! IPv4 address.
    ROC_AF_IPv4 = 1,

    //! IPv6 address.
    ROC_AF_IPv6 = 2
} roc_family;

enum {
    //! Address struct size.
    ROC_ADDRESS_SIZE = 64
};

//! Network address.
typedef struct roc_address {
#ifndef ROC_DOXYGEN
    // Private data. Do not use directly.
    union {
        unsigned long align;
        char payload[ROC_ADDRESS_SIZE];
    } private_data;
#endif // ROC_DOXYGEN
} roc_address;

//! Init address.
ROC_API int
roc_address_init(roc_address* address, roc_family family, const char* ip, int port);

//! Get address family.
ROC_API roc_family roc_address_family(const roc_address* address);

//! Get address IP address.
ROC_API const char* roc_address_ip(const roc_address* address, char* buf, size_t bufsz);

//! Get address port.
ROC_API int roc_address_port(const roc_address* address);

#ifdef __cplusplus
}
#endif

#endif // ROC_ADDRESS_H_
