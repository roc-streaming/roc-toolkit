/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_status/status_code.h
//! @brief Status codes.

#ifndef ROC_STATUS_STATUS_CODE_H_
#define ROC_STATUS_STATUS_CODE_H_

namespace roc {
namespace status {

//! Status code.
enum StatusCode {
    //! Uninitialized status value.
    //! @remark
    //!  Should be never returned from anywhere.
    //!  Indicates that we forgot to assign actual status to a variable
    //!  before returning it.
    NoStatus = -1,

    //! Operation completed successfully.
    StatusOK,

    //! Returned less data than requested.
    //! @remarks
    //!  Indicates that the returned data is shorter than requested. This can
    //!  happen due to some sort of buffering or batching and does NOT mean
    //!  that there is no more data in stream.
    //! @note
    //!  Example: we've requested 100 samples, but got only 60. We are allowed
    //!  to request 40 more samples if we need it.
    //! @pre
    //!  This status can be returned only from read operation.
    StatusPart,

    //! Stream is empty currently, but more to come later.
    //! @remarks
    //!  Indicates that we can't read more data right now and should try later,
    //!  when more data arrives.
    //! @note
    //!  Example: we've read all packets from incoming queue and it became
    //!  empty (drained), but more packets are expected.
    //! @pre
    //!  This status can be returned only from read operation.
    StatusDrain,

    //! Stream aborted prematurely.
    //! @remarks
    //!  Indicates that we've can't read or write anymore because stream
    //!  was abnormally interrupted and terminated.
    //! @note
    //!  Example: session terminated because of no_playback timeout.
    StatusAbort,

    //! Stream is fully read or written.
    //! @remarks
    //!  Indicates that we've successfully read or write everything and there is
    //!  no more data expected.
    //! @note
    //!  Example: we've got end of file when reading from file, or end of
    //!  stream when reading from network.
    StatusFinish,

    //! Insufficient memory.
    //! @remarks
    //!  Indicates low memory or reached memory limit.
    //! @note
    //!  Example: not enough memory when creating new session.
    StatusNoMem,

    //! No route found.
    //! @remarks
    //!  Indicates that there is no suitable route to handle request.
    //! @note
    //!  Example: we're trying to write a packet, but there is no exiting session
    //!  to which it belongs.
    StatusNoRoute,

    //! No driver found.
    //! @remarks
    //!  Indicates that there is no suitable driver to open sink or source.
    //! @note
    //!  Example: we're trying to open a pulseaudio device using a backend
    //!  that supports only alsa devices.
    StatusNoDriver,

    //! Unsupported format.
    //! @remarks
    //!  Indicates that the format or sub-format requested is not supported.
    //! @note
    //!  Example: we're trying to open an mp3 file using a backend that
    //!  supports only wav files.
    StatusNoFormat,

    //! Unusable or missing plugin.
    //! @remarks
    //!  Indicates that plugin lookup or initialization failed.
    //! @note
    //!  Example: we're trying to create PLC plugin, but user-provided callback
    //!  failed to allocate it.
    StatusNoPlugin,

    //! Failure with audio device.
    //! @remarks
    //!  Indicates that error occurred when working with audio device.
    //! @note
    //!  Example: can't open device, can't write to device.
    StatusErrDevice,

    //! Failure with file.
    //! @remarks
    //!  Indicates that error occurred when working with file.
    //! @note
    //!  Example: can't open file, can't write to file.
    StatusErrFile,

    //! Failure with networking.
    //! @remarks
    //!  Indicates that error occurred when trying to perform network operation.
    //! @note
    //!  Example: can't create a socket or establish connection.
    StatusErrNetwork,

    //! Failure with threads.
    //! @remarks
    //!  Indicates that error occurred when trying to start thread.
    //! @note
    //!  Example: can't start control loop thread because system limit reached.
    StatusErrThread,

    //! Failure with PRNG.
    //! @remarks
    //!  Indicates that error occurred when working PRNG.
    //! @note
    //!  Example: can't read bytes from CSPRNG.
    StatusErrRand,

    //! Bad slot state.
    //! @remarks
    //!  Slot state doesn't allow operation.
    //! @note
    //!  Example: trying to use slot that was marked broken.
    StatusBadSlot,

    //! Bad interface state.
    //! @remarks
    //!  Interface state doesn't allow operation.
    //! @note
    //!  Example: trying to use interface that was not activated.
    StatusBadInterface,

    //! Bad protocol value.
    //! @remarks
    //!  Requested protocol is not allowed or supported in this context.
    //! @note
    //!  Example: trying use transport protocol with control interface, or
    //!  trying to connect using a protocol that supports only binding.
    StatusBadProtocol,

    //! Bad configuration.
    //! @remark
    //!  Failure caused by improper or inconsistent configuration.
    //! @note
    //!  Example: config fields have invalid values or are not consistent
    //!  with each other.
    StatusBadConfig,

    //! Malformed packet.
    //! @remarks
    //!  Indicates the provided packet has invalid or unsupported format
    //!  and can't be parsed.
    //! @note
    //!  Example: RTP packet doesn't have proper version in corresponding
    //!  header field.
    StatusBadPacket,

    //! Provided buffer has inappropriate size.
    //! @remarks
    //!  Indicates the output buffer provided by user is insufficient to hold
    //!  result, or input buffer provided by user is larger than allowed
    //!  maximum, or buffer size does not fulfill other requirements.
    //! @note
    //!  Example: user tries to read packet into a buffer, but packet is
    //!  larger than the buffer; user tries to write frame, but frame
    //!  size is not multiple of sample size.
    StatusBadBuffer,

    //! Illegal argument.
    //! @remark
    //!  One of the provided function arguments has invalid value.
    //! @note
    //!  Example: passing null pointer when it's not allowed, passing
    //!  invalid enum value.
    StatusBadArg,

    //! Illegal operation.
    //! @remark
    //!  Operation is not allowed or supported in this context.
    //! @note
    //!  Example: trying to push packet for an interface that does not support
    //!  it, trying to connect using a protocol that doesn't support it.
    StatusBadOperation,

    //! Illegal object state.
    //! @remark
    //!  Object state is invalid and object can't be used anymore.
    //! @note
    //!  Example: trying to write frame after previous write failed.
    StatusBadState,

    //! Maximum enum value.
    MaxStatus,
};

} // namespace status
} // namespace roc

#endif // ROC_STATUS_STATUS_CODE_H_
