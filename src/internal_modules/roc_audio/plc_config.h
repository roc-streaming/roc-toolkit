/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/plc_config.h
//! @brief PLC config.

#ifndef ROC_AUDIO_PLC_CONFIG_H_
#define ROC_AUDIO_PLC_CONFIG_H_

#include "roc_core/attributes.h"

namespace roc {
namespace audio {

//! PLC backends.
enum PlcBackend {
    //! Use default PLC.
    PlcBackend_Default,

    //! Disable PLC.
    PlcBackend_None,

    //! Insert loud beep instead of losses.
    PlcBackend_Beep,

    //! Maximum enum value.
    PlcBackend_Max
};

//! PLC config.
struct PlcConfig {
    //! PLC backend.
    //! May be set to one of the PlcBackend values, or to custom backend id
    //! in range [MinBackendId; MaxBackendId] registered in ProcessorMap.
    int backend;

    PlcConfig()
        : backend(PlcBackend_Default) {
    }

    //! Automatically fill missing settings.
    ROC_ATTR_NODISCARD bool deduce_defaults();
};

//! Get string name of PLC backend.
const char* plc_backend_to_str(PlcBackend backend);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PLC_CONFIG_H_
