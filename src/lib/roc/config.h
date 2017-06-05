/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief Roc configuration.

#ifndef ROC_CONFIG_H_
#define ROC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ROC_API_CONF_DISABLE_FEC 1
#define ROC_API_CONF_LDPC_CODE 2
#define ROC_API_CONF_RS_CODE 0

typedef struct roc_config {
    unsigned int samples_per_packet;
    unsigned int n_source_packets;
    unsigned int n_repair_packets;

    unsigned int options;
} roc_config;

#ifdef __cplusplus
}
#endif

#endif // ROC_CONFIG_H_
