/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_controller.h
//! @brief SoX library controller.

#ifndef ROC_SNDIO_SOX_CONTROLLER_H_
#define ROC_SNDIO_SOX_CONTROLLER_H_

#include <sox.h>

#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"

namespace roc {
namespace sndio {

//! SoxController.
class SoxController : public core::NonCopyable<> {
public:
    //! Get controller instance.
    static SoxController& instance() {
        return core::Singleton<SoxController>::instance();
    }

    //! Get global options.
    //! @remarks
    //!  Always access SoX globals via this method to ensure that they were
    //!  already properly initialized (in the singleton constructor).
    sox_globals_t& get_globals() const;

    //! Fill default driver and device if necessary.
    //! @remarks
    //!  Fills @p driver and @p device with platform-specific defaults.
    bool fill_defaults(const char*& driver, const char*& device);

private:
    friend class core::Singleton<SoxController>;

    SoxController();
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_CONTROLLER_H_
