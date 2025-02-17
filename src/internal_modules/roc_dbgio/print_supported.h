/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_dbgio/print_supported.h
//! @brief Print supported protocols, formats, etc.

#ifndef ROC_DBGIO_PRINT_SUPPORTED_H_
#define ROC_DBGIO_PRINT_SUPPORTED_H_

#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_sndio/backend_dispatcher.h"

namespace roc {
namespace dbgio {

//! Print flags.
enum {
    Print_Netio = (1 << 0),
    Print_Sndio = (1 << 1),
    Print_Audio = (1 << 2),
    Print_FEC = (1 << 3),
};

//! Print supported protocols, formats, etc.
ROC_ATTR_NODISCARD bool print_supported(unsigned flags,
                                        sndio::BackendDispatcher& backend_dispatcher,
                                        core::IArena& arena);

} // namespace dbgio
} // namespace roc

#endif // ROC_DBGIO_PRINT_SUPPORTED_H_
