/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/endpoint.h
//! @brief Network endpoint.

#ifndef ROC_ADDRESS_ENDPOINT_H_
#define ROC_ADDRESS_ENDPOINT_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_uri.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/array.h"

namespace roc {
namespace address {

//! Network endpoint.
class Endpoint : public core::NonCopyable<> {
public:
    //! Initialize.
    Endpoint(core::IAllocator& allocator);

    //! Get endpoint URI.
    const EndpointURI& uri() const;

    //! Get mutable endpoint URI.
    EndpointURI& uri();

    //! Get multicast interface.
    //! @returns NULL if there is no multicast interface.
    const char* miface() const;

    //! Set multicast interface.
    //! @returns false on allocation error.
    bool set_miface(const char* miface);

    //! Get broadcast flag.
    bool broadcast() const;

    //! Set broadcast flag.
    void set_broadcast(bool);

private:
    EndpointURI uri_;
    core::Array<char> miface_;
    bool broadcast_;
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ENDPOINT_H_
