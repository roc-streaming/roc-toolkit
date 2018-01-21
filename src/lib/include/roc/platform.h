/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/platform.h
 * @brief Platform-specific definitions.
 */

#ifndef ROC_PLATFORM_H_
#define ROC_PLATFORM_H_

#include <stddef.h>

#if defined(__GNUC__)
/** Compiler attribute for an exported API function. */
#define ROC_API __attribute__((visibility("default")))
#else /* !__GNUC__ */
#error "unsupported compiler"
#endif /* __GNUC__ */

#endif /* ROC_PLATFORM_H_ */
