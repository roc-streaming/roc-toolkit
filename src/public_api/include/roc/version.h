/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/version.h
 * \brief Version numbers.
 */

#ifndef ROC_VERSION_H_
#define ROC_VERSION_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** First component of library version triple.
 */
#define ROC_VERSION_MAJOR 0

/** Second component of library version triple.
 */
#define ROC_VERSION_MINOR 4

/** Third component of library version triple.
 */
#define ROC_VERSION_PATCH 0

/** Convert version triple to numeric version code.
 * Version codes can be compared directly, e.g.:
 * \code
 *   #if ROC_VERSION < ROC_VERSION_CODE(1, 2, 3)
 *   ...
 *   #endif
 * \endcode
 */
#define ROC_VERSION_CODE(major, minor, patch) ((major)*1000000 + (minor)*1000 + (patch))

/** Numeric version code that corresponds to library version.
 * \see ROC_VERSION_CODE
 */
#define ROC_VERSION                                                                      \
    ROC_VERSION_CODE(ROC_VERSION_MAJOR, ROC_VERSION_MINOR, ROC_VERSION_PATCH)

/** Version components.
 */
typedef struct roc_version {
    /** First component of library version triple.
     */
    unsigned int major;

    /** Second component of library version triple.
     */
    unsigned int minor;

    /** Third component of library version triple.
     */
    unsigned int patch;

    /** Numeric version code.
     */
    unsigned int code;
} roc_version;

/** Retrieve version numbers.
 *
 * This function can be used to retrieve actual run-time version of the library.
 * It may be different from the compile-time version when using shared library.
 */
ROC_API void roc_version_load(roc_version* version);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_VERSION_H_
