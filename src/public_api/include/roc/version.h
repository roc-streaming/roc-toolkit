/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/version.h
 * \brief Roc version.
 */

#ifndef ROC_VERSION_H_
#define ROC_VERSION_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Major version component.
 */
#define ROC_VERSION_MAJOR 0

/** Minor version component.
 */
#define ROC_VERSION_MINOR 2

/** Patch version component.
 */
#define ROC_VERSION_PATCH 0

/** Version components.
 */
typedef struct roc_version {
    /** Major version component.
     */
    unsigned int major;

    /** Minor version component.
     */
    unsigned int minor;

    /** Patch version component.
     */
    unsigned int patch;
} roc_version;

/** Retrieve version numbers.
 *
 * Unlike \c ROC_VERSION_ macros, this function can be uses to
 * retrieve run-time version instead of compile-time one.
 */
ROC_API void roc_get_version(roc_version* version);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_VERSION_H_
