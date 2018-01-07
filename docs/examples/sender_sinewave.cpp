/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>


int main()
{
	// Destination address.
	const char* dst_addr_str = "127.0.0.1:12345";
	// The number of samples in single packet.
	const size_t packet_sz = 640;

	roc_config conf;
	memset(&conf, 0, sizeof(roc_config));
	conf.options = 0; // Synchronous variant is a default.
	conf.FEC_scheme = roc_config::ReedSolomon2m; // Enable Forward Error Correction.

	conf.samples_per_packet = packet_sz/2; 	// Each packet consists 320 samples of left channel 
						// and 320 samples of right channel.

	conf.n_source_packets = 10;		// For every 10 packets with audio samples, 
	conf.n_repair_packets = 5;		// Roc will send 5 more redundant packets
						// to be able to withstand congestions and losts.


	roc_sender* sndr = roc_sender_new(&conf);
	// Bind sender to a destination.
	// Important: Roc uses RTP packets over UDP, so it isn't informed if the receiver
	// is actually exists.
	roc_sender_bind(sndr, dst_addr_str);

	// Buffer for samples that we'll fill with a sine-waveSen
	float samples[packet_sz];

	// Time counter.
	size_t t = 0;

	// Sampling frequency.
	const double Fs = 44100;

	// Sine-wave frequency.
	const double F = 440;

	// For each packet:
	for (size_t i = 0; i < 100; ++i) {
		// For each sample in i-th packet:
		for (size_t j = 0; j < packet_sz/2; ++j) {
			// Left channel.
			samples[j*2] = sin(2*M_PI * F/Fs * (double)(t));
			// Right channel.
			samples[j*2 + 1] = -samples[j*2];
			t += 1;
		}
		roc_sender_write(sndr, samples, packet_sz);
	}

	roc_sender_delete(sndr);

	return 0;	
}
