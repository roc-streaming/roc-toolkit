Audio backends
**************

.. contents:: Table of contents:
   :local:
   :depth: 1

I/O URI
=======

Input/output device or file is identified via I/O URI, which have one of the following forms:

========================== ========================== ==============
syntax                     meaning                    example
========================== ========================== ==============
``<driver>://<device>``    audio device name          ``alsa://hw:0``
``<driver>://default``     default audio device       ``pulse://default``
``file://<abs_path>``      audio file (absolute path) ``file:///home/user/test.wav``
``file:<rel_path>``        audio file (relative path) ``file:./test.wav``
``file://-`` or ``file:-`` stdin or stdout            ``file:-``
========================== ========================== ==============

User can specify input file/device (**source**) for ``roc-send`` via ``--input`` option, and output file/device (**sink**) for ``roc-recv`` via ``--output`` option.

When device is used, user specifies driver explicitly (e.g. ``alsa://`` for ALSA, ``pulse://`` for PulseAudio, etc). When file is used, file driver is selected automatically, usually by file extension. However, user may force usage of specific driver for the file via ``--input-format`` or ``--output-format`` option.

See :doc:`manual pages </manuals>` for more details.

Sinks and sources
=================

To abstract files and devices, ``roc_sndio`` module defines three interfaces:

* device interface (`IDevice <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1IDevice.html>`_) - parent interface for input/output file/device
* source interface (`ISource <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1ISource.html>`_) - child interface for input file/device
* sink interface (`ISink <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1ISink.html>`_) - child interface for output file/device

For different types of devices and files, there may be different implementations of ``ISource`` and ``ISink``.

The same two interfaces (``ISource`` and ``ISink``) are also implemented by sender pipeline (`SenderSink <https://roc-streaming.org/toolkit/doxygen/classroc_1_1pipeline_1_1SenderSink.html>`_) and receiver pipeline (`ReceiverSource <https://roc-streaming.org/toolkit/doxygen/classroc_1_1pipeline_1_1ReceiverSource.html>`_). See more details in :doc:`/internals/pipelines`.

The job of ``roc-send`` and ``roc-recv`` is thus to open a source and a sink and to transfer audio from source to sink:

- in ``roc-send``, ``ISource`` is implemented by device or file from ``roc_sndio``, and ``ISink`` is implemented by sender pipeline from ``roc_pipeline``

- in ``roc-recv``, ``ISource`` is implemented by receiver pipeline from ``roc_pipeline``, and ``ISink`` is implemented by device or file from ``roc_sndio``

The task of transferring sound from ``ISource`` to ``ISink`` is implemented in `sndio::IoPump <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1IoPump.html>`_ class, which works uniformly with any pair of source and sink, being it file, device, or pipeline.

Backends and drivers
====================

Top-level class of ``roc_sndio`` is **backend dispatcher** (`BackendDispatcher <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1BackendDispatcher.html>`_), which holds all registered backends and decides which backend to use when opening sink or source.

Every **backend** (`IBackend <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1IBackend.html>`_) may implement one or more **drivers**, identified by string name.

For example, SoxBackend (backend that implements audio I/O using SoX library) implements several device and file drivers: ``alsa``, ``pulse``, ``mp3``, etc. Every device driver corresponds to particular sound system (e.g. ALSA, PulseAudio), and every file driver corresponds to particular file format (e.g. MP3).

When user asks backend dispatcher to open sink or source, user specifies I/O URI and, for files, optional file format (i.e. driver name). Backend dispatcher then finds backend which is able to handle given device or file and asks it to create its implementation of ``ISource`` or ``ISink``.

To help backend dispatcher with making decision, backends provide **driver information** (`DriverInfo <https://roc-streaming.org/toolkit/doxygen/classroc_1_1sndio_1_1DriverInfo.html>`_) about every available driver:

* *driver name* -- short unique string, for devices same as URI scheme (e.g. ``alsa``), for files usually same as file extension (e.g. ``mp3``)
* *driver type* -- whether it's device or file driver
* *driver flags* -- whether driver supports sources and/or sinks, and whether driver should be used by default

Supported backends
==================

To get list of supported device and file drivers, run ``roc-recv`` or ``roc-send`` with ``--list-supported`` option.

Most backends may be disabled at compile-time, so this list depends on build options.

The following table lists implemented audio backends:

==================== =========== ===============
backend              drivers     description
==================== =========== ===============
PulseaudioBackend    ``pulse``   native PulseAudio backend using libpulse
SndfileBackend       many        read and write various audio files using libsndfile
WavBackend           ``wav``     read and write audio files without external dependencies
SoxBackend           many        universal backend that supports many audio systems and file formats using libsox
==================== =========== ===============
