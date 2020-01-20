/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_ADDRESS_HELPERS_H_
#define ROC_ADDRESS_HELPERS_H_

#include "roc/address.h"

#include "roc_address/socket_addr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace api {

const char* get_address_payload(const roc_address* address);

char* get_address_payload(roc_address* address);

const address::SocketAddr& get_socket_addr(const roc_address* address);

address::SocketAddr& get_socket_addr(roc_address* address);

} // namespace api
} // namespace roc

#endif // ROC_ADDRESS_HELPERS_H_
