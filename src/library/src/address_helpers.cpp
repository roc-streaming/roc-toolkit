/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "address_helpers.h"

namespace roc {
namespace api {

const char* get_address_payload(const roc_address* address) {
    return address->private_data.payload;
}

char* get_address_payload(roc_address* address) {
    return address->private_data.payload;
}

const address::SocketAddr& get_socket_addr(const roc_address* address) {
    return *(const address::SocketAddr*)get_address_payload(address);
}

address::SocketAddr& get_socket_addr(roc_address* address) {
    return *(address::SocketAddr*)get_address_payload(address);
}

} // namespace api
} // namespace roc
