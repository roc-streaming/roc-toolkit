/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/version.h
 * @brief Version macros.
 */

#ifndef ROC_VERSION_H_
#define ROC_VERSION_H_

#define ROC_VERSION_MAJOR 0
#define ROC_VERSION_MINOR 1
#define ROC_VERSION_PATCH 5

#ifdef __cplusplus
extern "C" {
#endif

// get version numbers
// fills major, minor, and patch if they're non-NULL
void roc_version(int* major, int* minor, int* patch);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // ROC_VERSION_H_
