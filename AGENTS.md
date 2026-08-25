AI agent instructions
=====================

Real-time audio streaming toolkit. Low-level, real-time, portable down to fairly old compilers and distros.

Repository layout
-----------------

    src/internal_modules/roc_*   internal C++ libraries (not public API)
    src/public_api/              libroc: C API headers, sources, examples
    src/tools/                   roc-send, roc-recv, roc-copy
    src/tests/                   unit tests, mirrors internal modules
    docs/sphinx/                 documentation
    scripts/                     build and maintenance helpers
    3rdparty/                    vendored/downloaded dependencies

Target directories
------------------

Platform- and dependency-specific code lives in `target_*` subdirectories inside a module.

SCons decides which targets are enabled and adds only those headers and sources to the build. Everything outside `target_*` must compile on every supported configuration.

    roc_core/
    ├── target_posix/roc_core/mutex.{h,cpp}
    ├── target_darwin/roc_core/time.{h,cpp}
    ├── array.h
    └── list.h

Some common targets: `target_pc`, `target_posix`, `target_posix_ext`, `target_posix_pc`, `target_gnu`, `target_darwin`, `target_android`.

Modules and layering
--------------------

Each module has its own namespace (`roc::core`, `roc::audio`, ...), its own include path (`roc_core/...`), and its own test directory (`src/tests/roc_core`)`.

    roc_core      containers, memory management, threading, primitives (STL replacement)
    roc_stat      statistical functions
    roc_status    error codes
    roc_address   network URIs and addresses
    roc_packet    network packets and packet processing
    roc_rtp       RTP
    roc_rtcp      RTCP
    roc_fec       FEC (FECFRAME)
    roc_sdp       SDP
    roc_audio     audio frames and frame processing
    roc_pipeline  chains of packet/frame processors
    roc_ctl       control loop (signaling, background tasks)
    roc_netio     network I/O loop
    roc_sndio     sound I/O loop
    roc_dbgio     debug dumps, temp files
    roc_node      top-level glue (context, sender, receiver)

Media flow:

- on receiver: netio → pipeline → sndio on a receiver
- on sender: sndio → pipeline → netio on a sender

Language subset
---------------

The build uses `-std=c++11` for C++ and `-std=c11` for C.

The existing code is still mostly written in C++98 style; we are gradually migrating, and new code may use the C++11 features listed below. Anything not listed stays off the table.

Allowed C++11 features:

- `#pragma once` (`scons fmt` inserts it into new headers)
- deleted members (`= delete`)
- `explicit operator bool()`
- `nullptr`
- `enum class`
- `static constexpr` for constants
- move constructors (typically only in `roc_core`)
- `static_assert`
- non-static member field initializers
- `override`, `final`

Avoid:

- exceptions
- `auto`, lambdas
- most of the STL: containers, iterators, smart pointers, chrono
- template specializations, function overloading, default arguments, user-defined literals, operator overloading
- copy and move constructors
- inheritance, besides interface inheritance

Use `roc_core` instead of the STL. It provides intrusive containers, explicit arena- and pool-based allocation, lock-free building blocks, no implicit heavy operations, runtime checks that panic on misuse.

Most classes are neither copyable nor movable — declare them so explicitly (`core::NonCopyable`). Only a handful of `roc_core` classes are RAII value types, e.g. `SharedPtr`.

Code style
----------

- Run `scons fmt` (clang-format plus a header fixer). It inserts the copyright header, the `//! @file` / `//! @brief` Doxygen block, and `#pragma once`. Existing headers with old-style `ROC_..._H_` guards are left as they are. `.fmtignore` excludes a file.
- 4 spaces, 1TBS: opening brace on the same line, braces even for single statements, body never on the control statement's line. 90 column limit.
- `SNAKE_CASE` macros, `CamelCase` types, `snake_case` functions/methods/fields/variables, trailing underscore on private methods and fields.
- Class member order: public types/constants, public methods; protected types/constants, protected methods; private types/constants, private methods, private fields.
- Trailing comments on `#endif // NAME`, `#else // !NAME`, and `} // namespace foo`.
- Doxygen comments on headers, classes, public members, and free functions. The build can turn missing docs into errors.
- Anonymous namespaces instead of static globals/functions/constants. Enums instead of defines. `const` wherever it carries meaning.
- Cover components with unit tests (CppUTest) under `src/tests/`, and cover features end-to-end with pipeline-level integration tests.
- The code must compile warning-free.

Error handling
--------------

There are no exceptions, so operations report failure through return codes:

- `roc_panic(...)` — a contract or invariant inside Roc is broken. Panicking is always better than a crash or UB. **Never panic on data from the outside world** (network, files, user input); panics are for our own bugs only.
- `roc_log(...)` — log every event and value someone would need to understand why an error happened. Be generous here; this is the main debugging tool in the field.
- `status::StatusCode` — the return value protocol of readers and writers, see below.

Memory management
-----------------

- Allocate from arenas and pools; never `new`/`delete` ad hoc.
- `packet::PacketFactory` and `audio::FrameFactory` wrap the pools for packets, frames, and their buffers.
- `core::Buffer` is refcounted; `core::Slice` is a refcounted (buffer, offset, length) view. Slices are how protocol layers nest (an outer composer sub-slices the buffer and hands the inner region to the nested composer) and how payload alignment is achieved.
- Containers are intrusive: an object carries its list/heap node, so enqueuing costs no allocation and no copy. This is what keeps queue operations lock-free.
- Nothing expensive should be implicit. Deep copies and allocations are explicit calls.

Glossary
--------

- **peer** — a sender or a receiver. **session** — one sender connected to one receiver, represented on both sides (`SenderSession`, `ReceiverSession`).
- **slot** — a group of endpoints on a sender or receiver, at most one of each type; lets a single peer serve several protocol combinations or several remote peers.
- **endpoint** — one network address/protocol within a slot. Three types: *source* (media), *repair* (FEC), *control* (RTCP).
- **source packet** — encoded media, possibly with a FEC payload ID. **repair packet** — redundancy data. **control packet** — out-of-band signaling.
- **SSRC** — synchronization source, identifies a stream within a session. **seqnum** — packet index within a stream. Both start at a random value.
- **FEC block** — a batch of source packets plus the repair packets encoded from them; encoding and decoding happen per block. **FEC payload ID** — header/footer giving a packet's position within its block.
- **FECFRAME** — the spec family integrating FEC schemes into RTP.

Internals docs
--------------

When you need to know any of the things listed below, read the corresponding page first.

- `docs/sphinx/internals/packets_frames.rst` — who owns a packet or frame after a call, fetch/peek and hard/soft read modes, status codes, and raw vs PCM vs opaque frames.
- `docs/sphinx/internals/pipelines.rst` — pipeline structure and element order, slots, endpoints, sessions, pipeline timing, and pipeline tasks.
- `docs/sphinx/internals/threads.rst` — which loop owns which thread, and how packets, frames, and tasks cross between them.
- `docs/sphinx/internals/timestamps.rst` — STS, CTS, RTS, QTS, latency estimation, and the RTCP timestamp mapping.
- `docs/sphinx/internals/fec.rst` — FECFRAME schemes, FEC block layout, and payload IDs.
- `docs/sphinx/internals/fe_resampler.rst` — why clock drift needs a resampler, the latency monitor → frequency estimator → resampler chain, and resampler backend tradeoffs.
- `docs/sphinx/internals/audio_backends.rst` — how `IoPump` moves audio between any `ISource` and `ISink`, including pipelines, and how a backend and driver get picked.
- `docs/sphinx/internals/network_protocols.rst` — which RFC governs each supported protocol (RTP, RTCP XR blocks, FECFRAME schemes).

Build and portability docs
--------------------------

When you change user-facing build options, required dependency versions, supported platforms, CI coverage, and things like that, check these pages and update them if necessary:

- `docs/sphinx/building/scons_options.rst`
- `docs/sphinx/building/dependencies.rst`
- `docs/sphinx/building/user_cookbook.rst`
- `docs/sphinx/building/developer_cookbook.rst`
- `docs/sphinx/portability/supported_platforms.rst`
- `docs/sphinx/portability/cross_compiling.rst`
- `docs/sphinx/portability/android_environment.rst`
- `docs/sphinx/development/continuous_integration.rst`
