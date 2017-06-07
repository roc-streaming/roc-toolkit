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

//! Receiver and sender options.
enum RocConfigOpt_t {
    //! Turn off resampler in receiver. Might be usefull for
    //! exact samples transmition.
    ROC_API_CONF_RESAMPLER_OFF = (1 << 0),

    //! Turn off interleaver in sender so as to preserve packets order.
    ROC_API_CONF_INTERLEAVER_OFF = (1 << 1),

    //! Disables timing in receiver or sender to let user use in synchronous environment.
    //! See further explanaion and usage examples of both cases in documentation.
    ROC_API_CONF_DISABLE_TIMING = (1 << 2)
};

//! Configuration of sender or receiver.
typedef struct roc_config {
    //! Number of samples of both channels in every packet.
    unsigned int samples_per_packet;

    //! Number of data packets per block.
    unsigned int n_source_packets;

    //! Number of repaier packets (FEC_scheme mustn't be NO_FEC) per block.
    unsigned int n_repair_packets;

    //! Session latency as number of samples.
    unsigned int latency;
    //! Timeout after which session is terminated as number of samples.
    unsigned int timeout;

    //! FEC scheme type.
    enum { ReedSolomon2m = 0, LDPC, NO_FEC } FEC_scheme;

    //! Containment for RocConfigOpt_t flags.
    int options;
} roc_config;

#ifdef __cplusplus
}
#endif

#endif // ROC_CONFIG_H_
