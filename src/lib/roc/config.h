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

enum RocConfigOpt_t {
    ROC_API_CONF_DISABLE_FEC = 1,
    ROC_API_CONF_LDPC_CODE = 2,
    ROC_API_CONF_RESAMPLER_OFF = 4,
    ROC_API_CONF_INTERLEAVER_OFF = 8,
    ROC_API_CONF_DISABLE_TIMING = 16
};

typedef struct roc_config {
    unsigned int samples_per_packet;
    unsigned int n_source_packets;
    unsigned int n_repair_packets;
    //! Session latency as number of samples.
    unsigned int latency;
    //! Timeout after which session is terminated as number of samples.
    unsigned int timeout;

    int options;
} roc_config;

#ifdef __cplusplus
}
#endif

#endif // ROC_CONFIG_H_
