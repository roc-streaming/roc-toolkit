/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/address.h
 * @brief Network address.
 */

#ifndef ROC_ADDRESS_H_
#define ROC_ADDRESS_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Network address family. */
typedef enum roc_family {
    /** Invalid address. */
    ROC_AF_INVALID = -1,

    /** Automatically detect address family from string format. */
    ROC_AF_AUTO = 0,

    /** IPv4 address. */
    ROC_AF_IPv4 = 1,

    /** IPv6 address. */
    ROC_AF_IPv6 = 2
} roc_family;

enum {
    /** Address struct size. */
    ROC_ADDRESS_SIZE = 256
};

/** Network address.
 *
 * Represents an Internet address, i.e. and IP address plus UDP or TCP port.
 * Similar to struct sockaddr.
 *
 * @b Thread-safety
 *  - should not be used concurrently
 */
typedef struct roc_address {
#ifndef ROC_DOXYGEN
    /* Private data. Do not use directly. */
    union {
        unsigned long align;
        char payload[ROC_ADDRESS_SIZE];
    } private_data;
#endif /* ROC_DOXYGEN */
} roc_address;

/** Initialize address.
 *
 * Parses an IP address from a string representation and initializes @p address.
 * If @p family is @c ROC_AF_AUTO, the address family is auto-detected from the @p ip
 * format. Otherwise, the @p ip format should correspond to the @p family specified.
 *
 * When @p address is used to bind a sender or receiver port, the "0.0.0.0" @p ip may
 * be used to bind the port to all network interfaces, and the zero @p port may be
 * used to bind the port to a randomly chosen ephemeral port.
 *
 * The user is responsible for allocating and deallocating @p address. An address
 * doesn't contain any dynamically allocated data, so no special deinitialization
 * is required.
 *
 * @b Parameters
 *  - @p address should point to a probably uninitialized struct allocated by user
 *  - @p family should be @c ROC_AF_AUTO, @c ROC_AF_IPv4, or @c ROC_AF_IPv6
 *  - @p ip should point to a zero-terminated string with a valid IPv4 or IPv6 address
 *  - @p port should be a port number in range [0; 65536)
 *
 * @b Returns
 *  - returns zero if @p address was successfully initialized
 *  - returns a negative value if the arguments are invalid
 */
ROC_API int
roc_address_init(roc_address* address, roc_family family, const char* ip, int port);

/** Get address family.
 *
 * @b Parameters
 *  - @p address should point to a properly initialized address struct
 *
 * @b Returns
 *  - returns the address family if no error occurred
 *  - returns @c ROC_AF_INVALID if the arguments are invalid
 */
ROC_API roc_family roc_address_family(const roc_address* address);

/** Get IP address.
 *
 * Formats the zero-terminated string representation of the IP address to the given
 * buffer. The function fails if the buffer is not large enough to store the string
 * plus the terminating zero.
 *
 * @b Parameters
 *  - @p address should point to a properly initialized address struct
 *  - @p buf should point to a probably uninitialized buffer allocated by user at least
 *    of the @p bufsz size
 *  - @p bufsz defines the @p buf size
 *
 * @b Returns
 *  - returns @p buf if the IP address was successfully stored into the @p buf
 *  - returns NULL if the buffer is too small to store the formatted IP address
 *  - returns NULL if the arguments are invalid
 */
ROC_API const char* roc_address_ip(const roc_address* address, char* buf, size_t bufsz);

/** Get address port.
 *
 * @b Parameters
 *  - @p address should point to a properly initialized address struct
 *
 * @b Returns
 *  - returns a non-negative port number if no error occurred
 *  - returns a negative value if the arguments are invalid
 */
ROC_API int roc_address_port(const roc_address* address);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_ADDRESS_H_ */
