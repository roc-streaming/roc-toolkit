Continuous integration
**********************

.. contents:: Table of contents:
   :local:
   :depth: 2

Overview
========

Travis is configured to build ``master`` and ``develop`` branches and pull requests.

Travis builds Roc for Linux and macOS. Linux worker uses Docker to run builds on several Linux distros. Linux worker also uses QEMU to run cross-compiled tests.

Docker images for continuous integration and cross-compilation are prepared using Docker Hub automated builds. They are based on official upstream images, adding pre-installed packages required for build. Dockerfiles for images are hosted in a separate GitHub repository. When a Dockerfile or an upstream image changes, Docker Hub automatically triggers rebuild.

Links:
 * `Travis project <https://travis-ci.org/roc-project/roc>`_
 * `Travis configuration <https://github.com/roc-project/roc/blob/master/.travis.yml>`_
 * `Docker Hub organization <https://hub.docker.com/u/rocproject/>`_
 * `Dockerfiles repo <https://github.com/roc-project/dockerfiles>`_

Docker images
=============

The following Docker images are used on our Travis builds.

Linux native
------------

=================================== ===================== ============= ===========================
Image                               Base image            Architecture  Compilers
=================================== ===================== ============= ===========================
rocproject/travis-ubuntu:18.04      ubuntu:18.04          x86_64        gcc-6, clang-6
rocproject/travis-ubuntu:16.04      ubuntu:16.04          x86_64        gcc-4.8, gcc-5, clang-3.7
rocproject/travis-ubuntu:14.04      ubuntu:14.04          x86_64        gcc-4.4, gcc-4.6, clang-3.4
rocproject/travis-debian            debian:stable         x86_64        distro default
rocproject/travis-fedora            fedora:latest         x86_64        distro default
rocproject/travis-centos            centos:latest         x86_64        distro default
rocproject/travis-opensuse          opensuse/leap:latest  x86_64        distro default
rocproject/travis-archlinux         archlinux/base:latest x86_64        distro default
rocproject/travis-alpine            alpine:latest         x86_64        distro default
=================================== ===================== ============= ===========================

Linux misc.
-----------

=================================== ===================== =======================================
Image                               Base image            Comment
=================================== ===================== =======================================
rocproject/travis-minimal           ubuntu:latest         minimal build environment
rocproject/travis-checks            ubuntu:19.04          sanitizers and valgrind
=================================== ===================== =======================================

Linux cross-compilation
-----------------------

======================================================== ============= =========
Image                                                    Architecture  Compilers
======================================================== ============= =========
rocproject/cross-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 armv6         gcc-4.7
rocproject/cross-arm-linux-gnueabihf:gcc-4.9             armv7         gcc-4.9
rocproject/cross-aarch64-linux-gnu:gcc-7.4               armv8         gcc-7.4
======================================================== ============= =========

Android cross-compilation
-------------------------

======================================== =========== =================================== =============
Image                                    APIs        ABIs                                Compilers
======================================== =========== =================================== =============
rocproject/cross-linux-android:ndk21     21-29       armeabi-v7a, arm64-v8a, x86, x86_64 clang-9.0.8
======================================== =========== =================================== =============

Run locally
===========

It is possible to run Docker-based builds locally, in the same environment as they are run on Travis.

For example, this will run Fedora build:

.. code::

   $ scripts/travis/docker.sh rocproject/travis-fedora scripts/travis/linux-x86_64/fedora.sh

You can also invoke Docker manually:

.. code::

    $ docker run -t --rm --cap-add SYS_PTRACE -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/travis-fedora \
          scons --build-3rdparty=openfec,cpputest --enable-debug test

Explanation:

* ``-t`` allocates a pseudo-TTY to enable color output
* ``--rm`` removes the container when the command exits
* ``--cap-add SYS_PTRACE`` enables ptracing which is needed for clang sanitizers
* ``-u "${UID}"`` changes the UID inside the container from root to the current user
* ``-v "${PWD}:${PWD}"`` mounts the current directory into the container at the same path
* ``-w "${PWD}"`` chdirs into that directory
