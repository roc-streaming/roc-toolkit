Continuous integration
**********************

.. contents:: Table of contents:
   :local:
   :depth: 2

Overview
========

GitHub Actions are configured to build ``master`` and ``develop`` branches and pull requests.

GitHub Actions build Roc for Linux and macOS. Linux worker uses Docker to run builds on several Linux distros. Linux worker also uses QEMU to run cross-compiled tests.

Docker images for continuous integration and cross-compilation are built using GitHub actions and pushed to Docker Hub. They are based on official upstream images, adding pre-installed packages required for our build. Dockerfiles for images are hosted in a separate GitHub repository.

Links:
 * `GitHub Actions page <https://github.com/roc-streaming/roc-toolkit/actions>`_
 * `GitHub Actions configuration <https://github.com/roc-streaming/roc-toolkit/blob/master/.github/workflows/build.yml>`_
 * `Docker Hub organization <https://hub.docker.com/u/rocstreaming/>`_
 * `Dockerfiles repo <https://github.com/roc-streaming/dockerfiles>`_

Docker images
=============

The following Docker images are used on our CI builds.

Linux native
------------

=================================== ===================== ============= ==================================
Image                               Base image            Architecture  Compilers
=================================== ===================== ============= ==================================
rocstreaming/env-ubuntu:24.04       ubuntu:24.04          x86_64        gcc-13, clang-15, clang-17
rocstreaming/env-ubuntu:22.04       ubuntu:22.04          x86_64        gcc-11, gcc-12, clang-11, clang-14
rocstreaming/env-ubuntu:20.04       ubuntu:20.04          x86_64        gcc-8, gcc-10, clang-8, clang-10
rocstreaming/env-ubuntu:18.04       ubuntu:18.04          x86_64        gcc-6, clang-6
rocstreaming/env-ubuntu:16.04       ubuntu:16.04          x86_64        gcc-4.8, clang-3.7
rocstreaming/env-ubuntu:14.04       ubuntu:14.04          x86_64        gcc-4.4, clang-3.4
rocstreaming/env-ubuntu:nolibs      ubuntu:latest         x86_64        distro default
rocstreaming/env-debian             debian:stable         x86_64        distro default
rocstreaming/env-fedora             fedora:latest         x86_64        distro default
rocstreaming/env-opensuse           opensuse/leap:latest  x86_64        distro default
rocstreaming/env-archlinux          archlinux/base:latest x86_64        distro default
rocstreaming/env-alpine             alpine:latest         x86_64        distro default
=================================== ===================== ============= ==================================

Linux toolchains
----------------

============================================================== ============= ====== ==========================
Image                                                          Architecture  Libc   Compilers
============================================================== ============= ====== ==========================
rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4               armv8-a       glibc  gcc-7.4, gcc-10.3
rocstreaming/toolchain-arm-linux-gnueabihf:gcc-4.9             armv7-a       glibc  gcc-4.9, gcc-7.4, gcc-10.3
rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 armv6         glibc  gcc-4.7
rocstreaming/toolchain-mips-openwrt-linux-atheros:17.01        mips32 24Kc   musl   gcc-5.4
rocstreaming/toolchain-mips-openwrt-linux-atheros:12.09        mips32 24Kc   uClibc gcc-4.6
============================================================== ============= ====== ==========================

Android toolchains
------------------

========================================== =========== =================================== =========
Image                                      APIs        ABIs                                Compilers
========================================== =========== =================================== =========
rocstreaming/toolchain-linux-android:ndk26 21-34       armeabi-v7a, arm64-v8a, x86, x86_64 clang-17
rocstreaming/toolchain-linux-android:ndk21 21-29       armeabi-v7a, arm64-v8a, x86, x86_64 clang-9
========================================== =========== =================================== =========

Full Android environment
------------------------

========================================== ===============================
Image                                      JDK
========================================== ===============================
rocstreaming/env-android:jdk15             openjdk:15.0.2-jdk-slim-buster
rocstreaming/env-android:jdk11             openjdk:11.0.7-jdk-slim-buster
rocstreaming/env-android:jdk8              openjdk:8u252-jdk-slim-buster
========================================== ===============================

How Docker images are built
===========================

`Docker images <https://github.com/roc-streaming/dockerfiles>`_ are built using `GitHub actions <https://github.com/roc-streaming/dockerfiles/blob/main/.github/workflows/build.yml>`_ and then pushed to Docker Hub.

Each image directory contains one or several dockerfiles and ``images.csv`` file in the following format:

.. code::

    DOCKERFILE;ARGS (comma-separated list);TAG

This file defines what tags to build, path to dockerfile and build arguments for each tag. Build arguments are passed as ARGs to ``docker build``.

If the value in the first column is left empty, it defaults to ``Dockerfile`` is in the same directory as ``images.csv``. If the value in the last column is omitted, it defaults to the name of the directory which contains Dockerfile, e.g. if Dockerfile path is ``14.04/Dockerfile``, then tag defaults to ``14.04``.

Example:

.. code::

    DOCKERFILE;ARGS (comma-separated list);TAG
    Dockerfile;MAJOR=4.9,MINOR=4,DATE=2017.01;gcc-4.9
    Dockerfile;MAJOR=7.4,MINOR=1,DATE=2019.02;gcc-7.4
    Dockerfile;MAJOR=7.4,MINOR=1,DATE=2019.02;latest

This file defines three tags: ``gcc-4.9``, ``gcc-7.4``, and ``latest``. Each tag uses the same ``Dockerfile`` and different arguments ``MAJOR``, ``MINOR``, and ``DATE``.

You can build an image(s) locally using:

.. code::

   ./make.sh [OPTIONS...] [IMAGE[:TAG]...]

For example, to build all tags of ``env-ubuntu`` image:

.. code::

   ./make.sh env-ubuntu

To build all tags of ``env-fedora`` image and two specific tags of ``env-ubuntu`` image:

.. code::

   ./make.sh env-fedora env-ubuntu:20.04 env-ubuntu:22.04

To build all images:

.. code::

   ./make.sh

For the full list of available options, run:

.. code::

   ./make.sh --help

Running CI steps locally
========================

Run Linux native tests locally
------------------------------

CI steps for various Linux distros are fully dockerized and don't depend on GitHub Actions environment. It's easy to run them locally in exactly same environment as on CI.

You can find specific commands to run in ``build.yml`` file. Look for images that are named ``rocstreaming/env-*``. For example, this command will run Fedora build:

.. code::

   $ scripts/ci_checks/docker.sh rocstreaming/env-fedora \
       scripts/ci_checks/linux-x86_64/fedora.sh

Under the hood, this command will run scons in docker:

.. code::

    $ docker run -t --rm --cap-add SYS_PTRACE -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/env-fedora \
          scons --build-3rdparty=openfec,cpputest --enable-debug test

Explanation:

* ``-t`` allocates a pseudo-TTY to enable color output
* ``--rm`` removes the container when the command exits
* ``--cap-add SYS_PTRACE`` enables ptracing which is needed for clang sanitizers
* ``-u "${UID}"`` changes the UID inside the container from root to the current user
* ``-v "${PWD}:${PWD}"`` mounts the current directory into the container at the same path
* ``-w "${PWD}"`` chdirs into that directory

Run Linux QEMU tests locally
----------------------------

There are CI steps that do cross-compilation and then run tests in QEMU (in user space mode, i.e. on host kernel).

These steps are also fully dockerized and you can run them locally. They use docker images that have both cross-compilation toolchain and QEMU pre-installed.

You can find specific commands to run in ``build.yml`` file. Look for images that are named ``rocstreaming/toolchain-*``. For example, this command will run ARM64 build:

.. code::

   $ scripts/ci_checks/docker.sh rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4 \
       scripts/ci_checks/linux-arm/aarch64-linux-gnu-gcc-7.4.sh

For more details, see :ref:`qemu`.

Run Android tests locally
-------------------------

CI steps for Android use emulator to run tests. You can do roughtly the same locally using ``android_emu.sh`` script:

.. code::

   $ scripts/android_emu.sh test

This command will pull ``rocstreaming/env-android`` Docker image, install necessary Android components inside it, build Roc, start Android emulator, and run Roc tests on emulator.

For more details, see :ref:`android_docker`.
