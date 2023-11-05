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
#define ROC_VERSION_PATCH 6

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
 * This function can be used to retrieve actual run-time version of the library.
 * It may be different from the compile-time version when using shared library.
 */
ROC_API void roc_version_get(roc_version* version);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_VERSION_H_
