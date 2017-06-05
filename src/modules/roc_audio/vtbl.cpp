/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"
#include "roc_audio/isink.h"
#include "roc_audio/istream_reader.h"

namespace roc {
namespace audio {

IStreamReader::~IStreamReader() {
}

ISampleBufferReader::~ISampleBufferReader() {
}

ISampleBufferWriter::~ISampleBufferWriter() {
}

ISink::~ISink() {
}

} // namespace audio
} // namespace roc
