Changelog
*********

.. contents:: Releases:
   :local:
   :depth: 1

..
    Features
    C API
    Command-line tools
    Bug fixes
    Portability
    Security
    Dependencies
    Internals
    Build system
    Packaging
    Tests
    Documentation

.. _v0.4.0:

Version 0.4.0 (Jun 14, 2024)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.4.0>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/16>`__

Features
--------

* finish RTCP & XR support (`gh-14 <https://github.com/roc-streaming/roc-toolkit/issues/14>`_, `gh-674 <https://github.com/roc-streaming/roc-toolkit/issues/674>`_)

  * handle all reports from RFC 3550 and some XR reports
  * implement 2-way RTCP reports exchange
  * route packets to sessions according to SSRC and CNAME
  * support RTT calculation based on RTCP XR packets
  * improve packet validation
  * support packet padding
  * support RTCP multicast

* support latency tuning and clock drift compensation on sender instead of receiver (`gh-675 <https://github.com/roc-streaming/roc-toolkit/issues/675>`_)
* implement backend for WAV files without external dependencies, useful in contrained environments (`gh-576 <https://github.com/roc-streaming/roc-toolkit/issues/576>`_)
* implement libsndfile backend for audio files (`gh-246 <https://github.com/roc-streaming/roc-toolkit/issues/246>`_)

C API
-----

* rework latency configuration, introduce concept of latency tuner, available on both sender and receiver (by default enabled on receiver and disabled on sender)

  * replace ``roc_clock_sync_backend`` with ``roc_latency_tuner_backend``
  * replace ``roc_clock_sync_profile`` with ``roc_latency_tuner_profile``

* rework metrics API (`gh-681 <https://github.com/roc-streaming/roc-toolkit/issues/681>`_)

  * replace ``roc_session_metrics`` with ``roc_connection_metrics``
  * rework ``roc_sender_metrics`` and ``roc_receiver_metrics``; connection metrics are now available on both sender and receiver
  * functions for querying metrics (``roc_sender_query()``, ``roc_receiver_query()``, ``roc_sender_encoder_query()``, ``roc_receiver_decoder_query()``) are reworked and simplified
  * ``niq_latency`` is removed from API

* support 2-way control packet exchange in codecs API (``roc_sender_encoder`` and ``roc_receiver_decoder``)

  * rename ``roc_sender_encoder_push()`` to ``roc_sender_encoder_push_frame()``
  * rename ``roc_sender_encoder_pop()`` to ``roc_sender_encoder_pop_packet()``
  * add ``roc_sender_encoder_push_feedback_packet()``
  * rename ``roc_receiver_decoder_push()`` to ``roc_receiver_decoder_push_packet()``
  * rename ``roc_receiver_decoder_pop()`` to ``roc_receiver_decoder_pop_frame()``
  * add ``roc_receiver_decoder_pop_feedback_packet()``

* rename ``roc_version_get()`` to ``roc_version_load()``

Command-line tools
------------------

* rework latency configuration:

  * rename ``--sess-latency`` to ``--target-latency``
  * replace ``--clock-backend`` with ``--latency-backend``
  * replace ``--clock-profile`` with ``--latency-profile``

* rename options:

  * ``--packet-length`` to ``--packet-len``
  * ``--packet-limit`` to ``--max-packet-size``
  * ``--frame-length`` to ``--frame-len``
  * ``--frame-limit`` to ``--max-frame-size``

* support floats for sizes and durations (`gh-654 <https://github.com/roc-streaming/roc-toolkit/issues/654>`_)
* support ``NO_COLOR`` and ``FORCE_COLOR`` environment variables (`gh-564 <https://github.com/roc-streaming/roc-toolkit/issues/564>`_)

Bug fixes
---------

* fix routing of FEC packets in encoder/decoder API (``roc_sender_encoder`` and ``roc_receiver_decoder``)
* fix segfault when roc-send input device omitted (`gh-728 <https://github.com/roc-streaming/roc-toolkit/issues/728>`_)
* fix work when ``no_playback_timeout`` is lower than ``target_latency`` (`gh-657 <https://github.com/roc-streaming/roc-toolkit/issues/657>`_)
* fix "unexpected already cancelled task" panic
* properly handle allocation errors in ``HeapArena``

Portability
-----------

* add Ubuntu 24.04 to CI (`gh-634 <https://github.com/roc-streaming/roc-toolkit/issues/634>`_)
* add macOS 14 (arm64) to CI
* add OpenWrt with uCLibc and musl (MIPS32) to CI
* fix build on Debian GNU/Hurd
* improve handling of unknown unix-like platforms in scons

Dependencies
------------

* new optional dependency: libsndfile, used in CLI tools (`gh-246 <https://github.com/roc-streaming/roc-toolkit/issues/246>`_)

Internals
---------

* support frame of different format in different parts of pipeline (`gh-547 <https://github.com/roc-streaming/roc-toolkit/issues/547>`_)
* continue work on configurable encoding (`gh-608 <https://github.com/roc-streaming/roc-toolkit/issues/608>`_)
* continue work on surround sound support (`gh-86 <https://github.com/roc-streaming/roc-toolkit/issues/86>`_)
* start work on configurable limits (`gh-610 <https://github.com/roc-streaming/roc-toolkit/issues/610>`_)
* improvements and refactoring in ``roc_core``

Build system
------------

* fix ``--build-3rdparty`` on macOS 14
* fix static library on macOS (``libroc.a``); ensure that all object files have unique names
* fix macos linker warnings about ``-lc++``
* fix build with macports installed; don't implicitly use brew if pkg-config is not from brew
* fix openssl search
* workaround for brew + pkg-config + openssl error on macOS
* fix building old pulseaudio on clang 17

Tests
-----

* improve tests for capture timestamps (CTS)
* improve RTCP tests (in ``roc_rtcp``, ``roc_pipeline``, and ``public_api``)
* improve pipeline tests
* add tests for metrics

Documentation
-------------

* document audio backends
* document sponsored work

.. _v0.3.0:

Version 0.3.0 (Nov 22, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.3.0>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/13>`__

Features
--------

* support lower latencies, up to 7ms in our tests
* add new clock synchronization profile (``responsive``) suitable for lower latencies
* major improvements in C API (network-less API, metrics API, many small improvements)
* more flexible packet encoding configuration (currently available only via C API):

  * more channel layouts: mono, stereo, multitrack (up to 1024 channels)
  * custom smaple rate

* improve scaling precision of ``speex`` resampler
* implement new ``speexdec`` resampler, combining SpeexDSP for base rate conversion and decimation for clock drift compensation, which has better scaling precision than ``speex`` and is very cheap when base rates are equal

C API
-----

* implement new encoder / decoder API (``roc_sender_encoder`` and ``roc_receiver_decoder``), which is network-less version of sender / receiver API (encoder produces packets, decoder consumes packets, and user is responsible for delivering packets)

* implement initial metrics API:

  * add ``roc_session_metrics``, ``roc_receiver_metrics``, and ``roc_sender_metrics`` structs (currently they support ``niq_latency`` and ``e2e_latency``)
  * add ``roc_sender_query()``, ``roc_receiver_query()``, ``roc_sender_encoder_query()``, ``roc_receiver_decoder_query()`` to query metrics

* improvements in slot support:

  * support deleting slots on fly using ``roc_sender_unlink()`` and ``roc_receiver_unlink()``
  * refine error handling rules: if error happens during slot configuration, slot is marked broken and excluded from pipeline, but needs manual removal by user
  * slot identifiers are now arbitrary ``long long`` numbers (not necessary continuous)

* simplify receiver configuration:

  * replace ``max_latency_overrun`` + ``max_latency_underrun`` with one parameter ``latency_tolerance``
  * rename ``broken_playback_timeout`` to ``choppy_playback_timeout``
  * remove ``breakage_detection_window`` (automatically derive it from ``choppy_playback_timeout``)

* simplify interface configuration:

  * introduce ``roc_interface_config`` struct, which holds all per-interface options
  * replace ``roc_sender_set_outgoing_address()`` and ``roc_sender_set_reuseaddr()`` with ``roc_sender_configure()`` (which uses ``roc_interface_config``)
  * replace ``roc_receiver_set_multicast_group()`` and ``roc_receiver_set_reuseaddr()`` with ``roc_receiver_configure()`` (which uses ``roc_interface_config``)

* rework encoding configuration:

  * rename ``roc_frame_encoding`` enum to ``roc_format`` (in API, "encoding" now means format + rate + channels, which is now true for both frame encoding and packet encoding)
  * add ``roc_media_encoding`` struct which defines format + rate + channels
  * use ``roc_media_encoding`` instead of ``frame_encoding`` + ``frame_sample_rate`` + ``frame_channels`` in ``roc_sender_config`` and ``roc_receiver_config``
  * rename ``roc_channel_set`` to ``roc_channel_layout``
  * remove ``packet_sample_rate`` and ``packet_channels`` (these parameters are now derived from ``packet_encoding``)
  * if ``packet_encoding`` is not set, automatically choose one that matches ``frame_encoding`` (among built-in and registered packet encodings)

* support channel layouts:

  * ``ROC_CHANNEL_LAYOUT_MONO``
  * ``ROC_CHANNEL_LAYOUT_STEREO``
  * ``ROC_CHANNEL_LAYOUT_MULTITRACK`` - up to 1024 channels without special meaning

* support packet encodings:

  * ``ROC_PACKET_ENCODING_AVP_L16_MONO``
  * ``ROC_PACKET_ENCODING_AVP_L16_STEREO``

* support registering custom packet encodings using ``roc_context_register_encoding()``

* add ``roc_clock_sync_backend`` parameter, with two values:

  * ``ROC_CLOCK_SYNC_BACKEND_DISABLE`` - do not adjust receiver clock
  * ``ROC_CLOCK_SYNC_BACKEND_NIQ`` - adjust receiver clock based on network incoming queue size (current behavior)

* add ``roc_clock_sync_profile`` parameter with three values:

  * ``ROC_CLOCK_SYNC_PROFILE_GRADUAL`` - adjust clock smoothly (old behavior, good for high jitter and high latency)
  * ``ROC_CLOCK_SYNC_PROFILE_RESPONSIVE`` - adjust clock smoothly (good for low jitter and low latency)
    ``ROC_CLOCK_SYNC_PROFILE_DEFAULT`` - select profile automatically based on ``target_latency``

* rename ``ROC_CLOCK_EXTERNAL`` / ``ROC_CLOCK_INTERNAL`` to ``ROC_CLOCK_SOURCE_EXTERNAL`` / ``ROC_CLOCK_SOURCE_INTERNAL``

* add ``ROC_RESAMPLER_BACKEND_SPEEXDEC`` backend

* add ``ROC_VERSION`` and ``ROC_VERSION_CODE()``

Command-line tools
------------------

* replace ``--min-latency`` + ``--max-latency`` with ``--latency-tolerance``
* remove ``--no-resampling`` (use ``--clock-backend=disable`` instead)
* add ``--clock-backend`` and ``--clock-profile``
* rename ``--np-timeout`` to ``--no-play-timeout``
* replace ``--bp-timeout`` + ``--bp-window`` with ``--choppy-play-timeout``
* rename ``--beeping`` to ``--beep``
* rename ``roc-conv`` tool to ``roc-copy``
* list supported endpoint schemes in ``--print-supported``

Bug fixes
---------

* fix NTP 2036 year problem
* fix latency reported in logs

Internals
---------

* change default packet length from ``7ms`` to ``2.5ms``
* get rid of hard-coded frame length in pipeline components (now they work with any requested frame length), which allows to handle latencies lower than default frame length
* support capture timestamps (CTS) in packets and frames (based on RTCP + NTP) and forward them through the pipeline, needed for end-to-end latency calculation
* start work on calculation of end-to-end latency (overall delay from sender to receiver, including I/O and network)
* start work for automatic mapping between different channel layouts and orders (including mono, stereo, surround, and multitrack layouts)
* start work for reporting and forwarding error codes through the pipeline
* implement fast lock-free PRNG
* optimize task processing: process pipeline tasks in-place when they're scheduled from I/O thread, to avoid unnecessary delays
* improve memory protection:

  * always employ memory poisoning in arenas and pools
  * implement buffer overflow protection using canary guards in arenas and pools
  * implement ownership checks in arenas and pools

* module ``roc_peer`` renamed to ``roc_node`` (because it now has non-peer nodes)

Build system
------------

* add ``--compiler-launcher`` scons option (may be used for ``ccache``)
* correctly handle ``--enable-debug-3rdparty`` for all dependencies

Documentation
-------------

* improve C API doxygen comments
* fix pulseaudio C API examples
* numerous improvements and updates in sphinx documentation

.. _v0.2.6:

Version 0.2.6 (Nov 05, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.6>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/15>`__

Packaging
---------

* build debian packages on debian:bullseye
* in debian packages, statically link all dependencies except ``libc``, ``libasound``, ``libpulse``
* ensure that packages are installable on debian:oldstable, debian:stable, ubuntu:20.04, ubuntu:22.04, ubuntu:latest

.. _v0.2.5:

Version 0.2.5 (Jul 28, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.5>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/14>`__

Bug fixes
---------

* fix byte order conversion

Build system
------------

* fix compiler type detection when compiler is specified via ``CC`` or ``CXX`` variable
* export symbols of dependencies built by ``--build-3rdparty`` when building static library (``libroc.a``), to avoid linker errors when using it

.. _v0.2.4:

Version 0.2.4 (May 13, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.4>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/12>`__

C API
-----

* always set ``file`` and ``line`` in ``roc_log_message``

Command-line tools
------------------

* support PulseAudio sources in ``roc-send``
* support ``--io-latency`` option in ``roc-send``

Bug fixes
---------

* fix potential race
* fix byte order detection on Android
* do not write to log from shared library destructor
* stop using user-provided log handler after entering shared library destructor

Internals
---------

* improve logging
* refactor scons scripts

Build system
------------

* fix ``--build-3rdparty=sox`` when ``sndio`` is installed
* fix ``--build-3rdparty=google-benchmark`` when there is ``python3``, but no ``python`` in PATH
* fix OpenSSL platform detection in ``--build-3rdparty=openssl`` when not cross-compiling
* set Android API level to ``21``
* add ``--macos-platform`` and ``--macos-arch`` scons options
* by default, set ``--macos-platform`` to current OS, to avoid linker warnings about incompatible macOS deployment targets
* support building macOS universal binaries by providing multiple values for ``--macos-arch``
* propagate Android platform, macOS platform, and macOS architectures to ``--build--3rdparty``
* unexport all symbols except ``roc_*`` from ``libroc.so`` and ``libroc.a`` on Linux, and ``libroc.dylib`` on macOS
* resolve ``pkg-config`` absolute path

Documentation
-------------

* minor updates

.. _v0.2.3:

Version 0.2.3 (Mar 09, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.3>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/11>`__

C API
-----

* add ``roc_receiver_set_reuseaddr`` and ``roc_sender_set_reuseaddr``

Command-line tools
------------------

* add ``--reuseaddr`` to ``roc-recv`` and ``roc-send``

Bug fixes
---------

* fix formatting of endpoint URI with zero port
* fix usage of multicast with RTCP in ``roc-recv``

Build system
------------

* add new dependency OpenSSL
* fix work with SCons 4.5
* exclude sox and libpulse from .pc file for libroc

Packaging
---------

* add debian packages and publish them on github
* add rpm packages spec

Documentation
-------------

* minor updates

.. _v0.2.2:

Version 0.2.2 (Feb 27, 2023)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.2>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/9>`__

C API
-----

* rename ``roc_get_version`` to ``roc_version_get``

Bug fixes
---------

* fix crash in ``roc_log_set_handler`` when argument is NULL

Build system
------------

* fix build on recent Android NDK
* install ``.pc`` file to ``<libdir>/pkgconfig`` instead of ``PKG_CONFIG_PATH``
* add support for ``DESTDIR``
* strip symbols in release build

Documentation
-------------

* minor updates

.. _v0.2.1:

Version 0.2.1 (Dec 26, 2022)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.1>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/10>`__

Build system
------------

* install to ``/usr`` by default (except macOS)

Documentation
-------------

* minor updates

.. _v0.2.0:

Version 0.2.0 (Dec 19, 2022)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.2.0>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/2>`__

Features
--------

* support multicast
* support broadcast
* support speex resampler and make it default
* support slots (connect sender to multiple receivers and vice versa)
* initial support for RTCP

C API
-----

* return error codes from ``roc_context_open``, ``roc_receiver_open``, ``roc_sender_open``
* introduce ``roc_endpoint`` to identify endpoints using URI
* rename ``roc_fec_code`` to ``roc_fec_encoding``
* add ``roc_resampler_backend``
* add ``roc_clock_source``
* add ``roc_version`` and friends

Command-line tools
------------------

* use URIs to identify audio devices and endpoints
* add ``--backup`` option to ``roc-recv``
* replace ``--frame-size`` with ``--frame-length`` and ``--frame-limit``
* remove ``--resampler-interp`` and ``--resampler-window``

Bug fixes
---------

* fix race in PRNG
* fix race in mutex and semaphore on macOS
* fix potential deadlock in network code

Portability
-----------

* Linux / aarch64 build fixes
* Android build fixes
* macOS build fixes
* FreeBSD build fixes
* support generic Unix target
* continuous integration for more Linux distros
* continuous integration for Android
* testing on Raspberry Pi 4

Internals
---------

* add ``roc_peer`` module
* add ``roc_ctl`` module
* support for asynchronous tasks in ``roc_pipeline``, ``roc_netio``, ``roc_ctl``
* lock-free task queues
* optimizations to avoid unnecessary context switches
* improvements in memory pools
* improvements in logger
* self-profiling
* start work on SDP support
* preparations for RTSP support
* rework project structure
* lots of small improvements

Build system
------------

* add ``--enable-static`` and ``--disable-shared``
* add ``--disable-soversion`` option
* compatibility with recent SCons versions
* compatibility with different Python versions
* improve toolchain detection
* generate ``.pc`` file for pkg-config
* fix build with recent PulseAudio
* fix build with recent libunwind
* fixes for building third-parties

Tests
-----

* add benchmarks
* lots of small updates

Documentation
-------------

* document Android bulding and testing
* lots of small updates

.. _v0.1.5:

Version 0.1.5 (Apr 05, 2020)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.5>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/7>`__

Portability
-----------

* fix building on Manjaro Linux
* fix building on Yocto Linux
* add openSUSE to continuous integration and user cookbook
* drop Xcode 7.3 from continuous integration, add Xcode 11.3

Build system
------------

* correctly handle arguments in environment variables like CXX/CC/LD/etc (for Yocto Linux)
* correctly handle spaces in environment variables (for Yocto Linux)
* fix environment overrides checks
* fix building of the host tools when cross-compiling
* fix warnings on Clang 11
* fix sphinx invocation
* explicitly disable Orc when building PulseAudio using --build-3rdparty
* explicitly enable -pthread or -lpthread for libsndfile (for Manjaro Linux)
* user CMake instead of autotools when building libuv for Android using ``--build-3rdparty``
* switch to libuv 1.35.0 by default in ``--build-3rdparty``
* check for unknown names in ``--build-3rdparty``

.. _v0.1.4:

Version 0.1.4 (Feb 06, 2020)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.4>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/6>`__

Internals
---------

* fix logging

Build system
------------

* make ``/usr/local`` prefix default everywhere except Linux
* make default compiler consistent with CXX var
* fix handling of RAGEL, GENGETOPT, DOXYGEN, SPHINX_BUILD, and BREATHE_APIDOC vars
* fix SoX download URL (again)
* fix CPU count calculation

Documentation
-------------

* update PulseAudio version numbers in "User cookbook"
* update CONTRIBUTING and "Coding guidelines"
* update maintainers and contributors list

.. _v0.1.3:

Version 0.1.3 (Oct 21, 2019)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.3>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/5>`__

Command-line tools
------------------

* add ``--list-drivers`` option
* add git commit hash to version info

Internals
---------

* print backtrace on Linux and macOS using libunwind instead of glibc backtrace module
* print backtrace on Android using bionic backtrace module
* colored logging

Build system
------------

* add libunwind optional dependency (enabled by default)
* add ragel required dependency
* rename "uv" to "libuv" in ``--build-3rdparty``
* don't hide symbols in debug builds
* strip symbols in release builds
* fix building on recent Python versions
* fix SoX download URL
* fix PulseAudio version parsing
* automatically apply memfd patch when building PulseAudio
* automatically fix libasound includes when building PulseAudio

.. _v0.1.2:

Version 0.1.2 (Aug 14, 2019)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.2>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/4>`__

Bug fixes
---------

* fix handling of inconsistent port protocols / FEC schemes
* fix IPv6 support
* fix incorrect usage of SO_REUSEADDR
* fix panic on bind error
* fix race in port removing code
* fix packet flushing mechanism
* fix backtrace printing on release builds

Portability
-----------

* fix building on musl libc
* continuous integration for Alpine Linux

Internals
---------

* rework audio codecs interfaces (preparations for Opus and read-aheads support)
* minor refactoring in FEC support
* improve logging

Build system
------------

* allow to configure installation directories
* auto-detect system library directory and PulseAudio module directory

Documentation
-------------

* extend "Forward Erasure Correction codes" page
* add new pages: "Usage", "Publications", "Licensing", "Contacts", "Authors"
* replace "Guidelines" page with "Contribution Guidelines", "Coding guidelines", and "Version control"

.. _v0.1.1:

Version 0.1.1 (Jun 18, 2019)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.1>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/3>`__

Bug fixes
---------

* fix memory corruption in OpenFEC / LDPC-Staircase (fix available in our fork)
* fix false positives in stream breakage detection

Portability
-----------

* start working on Android port; Roc PulseAudio modules are now available in Termux unstable repo
* continuous integration for Android / arm64 (minimal build)
* docker image for aarch64-linux-android toolchain

Build system
------------

* fix multiple build issues on macOS
* fix multiple build issues with cross-compilation and Android build
* fix issues with building third-parties
* fix issues with compilation db generation
* set library soname/install_name and install proper symlinks
* improve configuration options
* improve system type detection and system tools search
* improve scripts portability
* better handling of build environment variables

Tests
-----

* fix resampler AWGN tests
* add travis job to run tests under valgrind

.. _v0.1.0:

Version 0.1.0 (May 28, 2019)
============================

.. note::

  `github release <https://github.com/roc-streaming/roc-toolkit/releases/tag/v0.1.0>`__, `github milestone <https://github.com/roc-streaming/roc-toolkit/milestone/1>`__

Features
--------

* streaming CD-quality audio using RTP (PCM 16-bit stereo)
* maintaining pre-configured target latency
* restoring lost packets using FECFRAME with Reed-Solomon and LDPC-Staircase FEC schemes
* converting between the sender and receiver clock domains using resampler
* converting between the network and input/output sample rates
* configurable resampler profiles for different CPU and quality requirements
* mixing simultaneous streams from multiple senders on the receiver
* binding receiver to multiple ports with different protocols
* interleaving packets to increase the chances of successful loss recovery
* detecting and restarting broken streams

C API
-----

* initial version of transport API (roc_sender, roc_receiver)

Command-line tools
------------------

* initial version of command-line tools (roc-send, roc-recv, roc-conv)

Portability
-----------

* GNU/Linux support
* macOS support
* continuous integration for Ubuntu, Debian, Fedora, CentOS, Arch Linux, macOS
* continuous integration for x86_64, ARMv6, ARMv7, ARMv8
* toolchain docker images for arm-bcm2708hardfp-linux-gnueabi, arm-linux-gnueabihf, aarch64-linux-gnu
* testing on Raspberry Pi 3 Model B, Raspberry Pi Zero W, Orange Pi Lite 2
