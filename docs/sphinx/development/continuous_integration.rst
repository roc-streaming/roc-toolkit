Continuous integration
**********************

.. contents:: Table of contents:
   :local:
   :depth: 2

Overview
========

GitHub Actions are configured to build ``master`` and ``develop`` branches and pull requests.

GitHub Actions build Roc for Linux and macOS. Linux worker uses Docker to run builds on several Linux distros. Linux worker also uses QEMU to run cross-compiled tests.

Docker images for continuous integration and cross-compilation are prepared using Docker Hub automated builds. They are based on official upstream images, adding pre-installed packages required for build. Dockerfiles for images are hosted in a separate GitHub repository. When a Dockerfile or an upstream image changes, Docker Hub automatically triggers rebuild.

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

=================================== ===================== ============= ================================
Image                               Base image            Architecture  Compilers
=================================== ===================== ============= ================================
rocstreaming/env-ubuntu:20.04       ubuntu:20.04          x86_64        gcc-8, gcc-10, clang-8, clang-10
rocstreaming/env-ubuntu:18.04       ubuntu:18.04          x86_64        gcc-6, clang-6
rocstreaming/env-ubuntu:16.04       ubuntu:16.04          x86_64        gcc-4.8, clang-3.7
rocstreaming/env-ubuntu:14.04       ubuntu:14.04          x86_64        gcc-4.4, clang-3.4
rocstreaming/env-ubuntu-minimal     ubuntu:latest         x86_64        distro default
rocstreaming/env-debian             debian:stable         x86_64        distro default
rocstreaming/env-fedora             fedora:latest         x86_64        distro default
rocstreaming/env-centos             centos:latest         x86_64        distro default
rocstreaming/env-opensuse           opensuse/leap:latest  x86_64        distro default
rocstreaming/env-archlinux          archlinux/base:latest x86_64        distro default
rocstreaming/env-alpine             alpine:latest         x86_64        distro default
=================================== ===================== ============= ================================

Linux cross-compilation
-----------------------

============================================================== ============= =========
Image                                                          Architecture  Compilers
============================================================== ============= =========
rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 armv6         gcc-4.7
rocstreaming/toolchain-arm-linux-gnueabihf:gcc-4.9             armv7         gcc-4.9
rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4               armv8         gcc-7.4
============================================================== ============= =========

Android cross-compilation
-------------------------

========================================== =========== =================================== =============
Image                                      APIs        ABIs                                Compilers
========================================== =========== =================================== =============
rocstreaming/toolchain-linux-android:ndk21 21-29       armeabi-v7a, arm64-v8a, x86, x86_64 clang-9.0.8
========================================== =========== =================================== =============

Run locally
===========

It is possible to run Docker-based builds locally, in the same environment as they are run on CI.

For example, this will run Fedora build:

.. code::

   $ scripts/ci/docker-linux.sh rocstreaming/env-fedora scripts/ci/linux-x86_64/fedora.sh

You can also invoke Docker manually:

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
