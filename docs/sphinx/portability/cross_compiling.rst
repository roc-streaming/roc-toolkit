Cross-compiling
***************

.. contents:: Table of contents:
   :local:
   :depth: 1

Preparing environment
=====================

Cross-compiling environment includes three things:

* toolchain
* sysroot
* dependencies

Toolchain is a set of build tools, including compiler, linker, etc. Sysroot is a directory that contains a subset of the root filesystem of the target operating system. Target system headers, libraries and run-time object files will be searched for in there. Sysroot path is configured when building toolchain. Typically, sysroot is located in the toolchain directory.

You can build toolchain manually or use one of the prebuilt toolchains described below. You can cross-compile dependencies manually and install them into sysroot. Alternatively, you can let Roc to download and cross-compile dependencies automatically. In this case, dependencies are not installed into the sysroot.

If you want to build toolchain and dependencies manually, you can use tools like `crosstool-ng <http://crosstool-ng.github.io/>`_, `buildroot <https://buildroot.org/>`_, and `crossdev <https://wiki.gentoo.org/wiki/Cross_build_environment>`_.

SCons options
=============

Use ``--host`` option to specify the toolchain name. This option defines a prefix which is added to all build tools, which should be available in PATH. For example, ``--host=arm-linux-gnueabihf`` means that Roc will expect that ``arm-linux-gnueabihf-gcc``, ``arm-linux-gnueabihf-ld``, etc. are available in PATH.

Use ``--build-3rdparty`` option to let Roc to download and build dependencies. When it is used with ``--host``, Roc automatically uses the specified toolchain for dependencies.

If necessary, you can override build tool names and options by setting variables like ``CC`` and ``CFLAGS``.

See :doc:`/building/scons_options` page for the full list of options and variables.

.. _aarch64-linux-gnu:

Arm.com / Linaro ARMv8-A 64-bit toolchain
=========================================

Arm Developer Hub and Linaro project provide several `toolchains <https://www.linaro.org/downloads/>`_ for different architectures and GCC versions.

The ``aarch64-linux-gnu`` is a 64-bit ARMv8-A little-endian toolchain. It can be used with 64-bit ARMv8 boards, including 64-bit Raspberry Pi and Orange Pi models. It *can't* be used with ARMv6, ARMv7, and 32-bit ARMv8 boards, or 64-bit ARMv8 boards running 32-bit kernel.

Here is how you can build Roc with this toolchain using `rocstreaming/toolchain-aarch64-linux-gnu <https://hub.docker.com/r/rocstreaming/toolchain-aarch64-linux-gnu/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4 \
          scons \
            --host=aarch64-linux-gnu \
            --build-3rdparty=all

Alternatively, you can install the toolchain manually:

.. code::

    # setup directories
    $ TOOLCHAIN_DIR=/path/to/toolchain
    $ ROC_DIR=/path/to/roc

    # for Roc
    $ apt-get install g++ scons ragel gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # download toolchain
    $ wget http://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/aarch64-linux-gnu/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz
    $ tar -C "${TOOLCHAIN_DIR}" -Jf gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz
    $ export PATH="${TOOLCHAIN_DIR}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin:${PATH}"

    # build Roc
    $ cd "${ROC_DIR}"
    $ scons --host=aarch64-linux-gnu --build-3rdparty=all

.. _arm-linux-gnueabihf:

Arm.com / Linaro ARMv7-A 32-bit toolchain
=========================================

Arm Developer Hub and Linaro project provide several `toolchains <https://www.linaro.org/downloads/>`_ for different architectures and GCC versions.

The ``arm-linux-gnueabihf`` is a 32-bit ARMv7-A hard-float little-endian toolchain. It can be used with ARMv7 boards, including 32-bit Raspberry Pi and Orange Pi models. It also can be used with 32-bit and 64-bit ARMv8 boards running 32-bit kernels. It *can't* be used with ARMv6 boards, e.g. Raspberry Pi 1 or Raspberry Pi Zero.

Here is how you can build Roc with this toolchain using `rocstreaming/toolchain-arm-linux-gnueabihf <https://hub.docker.com/r/rocstreaming/toolchain-arm-linux-gnueabihf/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-linux-gnueabihf:gcc-4.9 \
          scons \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=all

Alternatively, you can install the toolchain manually:

.. code::

    # setup directories
    $ TOOLCHAIN_DIR=/path/to/toolchain
    $ ROC_DIR=/path/to/roc

    # for Roc
    $ apt-get install g++ scons ragel gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # download toolchain
    $ wget http://releases.linaro.org/components/toolchain/binaries/4.9-2016.02/arm-linux-gnueabihf/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf.tar.xz
    $ tar -C "${TOOLCHAIN_DIR}" -Jf gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf.tar.xz
    $ export PATH="${TOOLCHAIN_DIR}/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf/bin:${PATH}"

    # build Roc
    $ cd "${ROC_DIR}"
    $ scons --host=arm-linux-gnueabihf --build-3rdparty=all

.. _arm-bcm2708hardfp-linux-gnueabi:

Raspberry Pi ARMv6 BCM-2708 toolchain
=====================================

The official Raspberry Pi `tools <https://github.com/raspberrypi/tools>`_ repository contains several arm-bcm2708 prebuilt toolchains. BCM-2708 is a chip family which includes BCM-2835, BCM-2836, and BCM-2837 chips used in various Raspberry Pi models (see `RPi Hardware <https://elinux.org/RPi_Hardware>`_).

The ``arm-bcm2708hardfp-linux-gnueabi`` is a 32-bit ARMv6 hard-float toolchain. It can be used with ARMv6 BCM-2708 boards, including Raspberry Pi 1 and Raspberry Pi Zero. It also can be used with ARMv7 and 32-bit ARMv8 boards, including more recent Raspberry Pi models, since they are backwards-compatible, but but can't employ instructions specific for these architectures.

Here is how you can build Roc with this toolchain using `rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi <https://hub.docker.com/r/rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 \
          scons \
            --host=arm-bcm2708hardfp-linux-gnueabi \
            --build-3rdparty=all

Alternatively, you can install the toolchain manually:

.. code::

    # setup directories
    $ RPI_TOOLS_DIR=/path/to/toolchain
    $ ROC_DIR=/path/to/roc

    # for Roc
    $ apt-get install g++ scons ragel gengetopt

    # for 3rd-parties
    $ apt-get install libtool intltool autoconf automake make cmake

    # for toolchain
    $ dpkg --add-architecture i386
    $ apt-get update
    $ apt-get install -y libstdc++6:i386 libgcc1:i386 zlib1g:i386

    # install toolchain
    $ git clone https://github.com/raspberrypi/tools.git "${RPI_TOOLS_DIR}"
    $ export PATH="${RPI_TOOLS_DIR}/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin:${PATH}"

    # build Roc
    $ cd "${ROC_DIR}"
    $ scons --host=arm-bcm2708hardfp-linux-gnueabi --build-3rdparty=all

.. _mips-openwrt-linux-atheros:

OpenWrt Atheros MIPS32 24Kc toolchains
======================================

Here is how you can build Roc with prebuilt Artheos OpenWrt toolchains using `rocstreaming/toolchain-mips-openwrt-linux-atheros <https://hub.docker.com/r/rocstreaming/toolchain-mips-openwrt-linux-atheros/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-mips-openwrt-linux-atheros:17.01 \
          scons \
            --host=mips-openwrt-linux-musl \
            --build-3rdparty=all \
            --disable-libunwind \
            --disable-pulseaudio \
            --disable-sox

Currently two toolchains are packaged:

* ``17.01`` -- OpenWrt 17.01 / ar71xx / musl (`openwrt archive <https://archive.openwrt.org/releases/17.01.7/targets/ar71xx/generic/>`__)
* ``12.09`` -- OpenWrt 12.09 / ar71xx / uClibc (`openwrt archive <https://archive.openwrt.org/attitude_adjustment/12.09/ar71xx/generic/>`__)

Debian and Ubuntu toolchains
============================

Debian and Ubuntu provide packaged toolchains as well, described on the `CrossToolchains <https://wiki.debian.org/CrossToolchains>`_ page on Debian wiki.

The ``arm-linux-gnueabihf`` toolchain can be used with ARMv7 boards. However note that the resulting binaries will require recent Glibc and, for instance, won't run on Raspbian versions which have more outdated one.

Here is how you can build Roc with this toolchain on Ubuntu:

.. code::

    # enable armhf architecture
    $ dpkg --add-architecture armhf

    # add armhf sources (replace "trusty" with your distro release name)
    $ cat >> /etc/apt/sources.list
    deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports trusty-updates main restricted universe multiverse
    deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports trusty-security main restricted universe multiverse
    ^D

    # fetch armhf sources
    $ apt-get update

    # for Roc
    $ apt-get install g++ scons ragel gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # install toolchain
    $ apt-get install crossbuild-essential-armhf

    # build Roc
    $ cd /path/to/roc
    $ scons --host=arm-linux-gnueabihf --build-3rdparty=all

.. _aarch64-linux-android:

Android NDK toolchains
======================

`Android NDK <https://developer.android.com/ndk>`_ provides two ways to build native code for Android:

* use one of the prebuilt toolchains from Android NDK directly;
* or prepare `a standalone toolchain <https://developer.android.com/ndk/guides/standalone_toolchain>`_ in a separate directory; the second approach is declared obsolete.

For convenience, Roc supports both ways.

To build Roc for Android using a prebuilt toolchain from Android NDK, you can use `rocstreaming/toolchain-linux-android <https://hub.docker.com/r/rocstreaming/toolchain-linux-android/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=aarch64-linux-android28 \
            --build-3rdparty=libuv,openfec,speexdsp

Alternatively, you can install Android NDK manually and run:

.. code::

    # for Roc
    $ apt-get install g++ scons ragel gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # setup path
    $ export PATH="/PATH_TO_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:${PATH}"

    # build Roc
    $ cd /path/to/roc
    $ scons -Q \
        --disable-tools \
        --compiler=clang \
        --host=aarch64-linux-android28 \
        --build-3rdparty=libuv,openfec,speexdsp

Supported ``--host`` values are:

* ``aarch64-linux-android<API>`` (64-bit ARM)
* ``armv7a-linux-androideabi<API>`` (32-bit ARM)
* ``x86_64-linux-android<API>`` (64-bit Intel)
* ``i686-linux-android<API>`` (32-bit Intel)

Here ``<API>`` stands for the Android API level, e.g. 28. Each Android NDK version supports its own set of the API levels.

Building Roc with a standalone toolchain is similar to cross-compiling with any other toolchain:

* prepare a toolchain for desired ABI (target architecture) and API level, e.g. ``aarch64-linux-android``
* add toolchain to ``PATH``
* pass toolchain to scons using ``--host`` option, e.g. ``--host=aarch64-linux-android``

Since standalone toolchains are obsolete, Roc doesn't provide prebuilt Docker images for them.

.. _android_docker:

Android Docker image
====================

There are scripts for building Roc for Android and running tests on Android emulator. Everything is built and run inside Docker, so you don't need to install anything on your system, besides Docker.

To build Roc for Android, you can just run:

.. code::

   $ scripts/android_emu.sh build

This command will pull ``rocstreaming/env-android`` Docker image, install necessary Android components inside it, and build Roc. Build results will be available in ``./bin``, as usual.

To run Roc tests on Android emulator, use ``test`` command:

.. code::

   $ scripts/android_emu.sh test

This command will additionally start Android emulator, and run Roc tests on it.

.. warning::

   This command will automatically employ KVM-based hardware acceleration. If you're using VirtualBox, you should temporary stop it and unload its kernel drivers, because they can't work with KVM side-by-side. You can do it using ``systemctl`` command.

Subsequent runs will be much faster than the first one, because Docker container will remaing running in background, and downloaded Android components will be cached in Docker volume. You can remove Docker container and volume using ``purge`` command:

.. code::

   $ scripts/android_emu.sh purge

Here is the full list of available commands:

* ``build`` - build code
* ``test`` - build code and run tests
* ``clean`` - remove build artifacts
* ``purge`` - remove build artifacts and docker container

You can also configure build via environment variables:

.. code::

   $ export API=28
   $ export ABI=x86_64
   $ export NDK_VERSION=21.1.6352462
   $ export BUILD_TOOLS_VERSION=28.0.3
   $ export CMAKE_VERSION=3.10.2.4988404

   $ scripts/android_emu.sh build

For more details about ``rocstreaming/env-android`` image, see :doc:`/portability/android_environment`.

Running cross-compiled binaries on target
=========================================

To run compiled binaries on the target system, you should install necessary runtime dependencies.

If you build Roc dependencies manually and install them into sysroot, you should also install them on the target system.

If you let Roc to build its dependencies automatically using ``--build-3rdparty`` option, most of them are statically linked into the Roc binaries, but there are still a few dependencies that are linked dynamically and so needed to be installed on the target system.

You can either copy their binaries from ``3rdparty/<toolchain>/rpath`` directory or obtain them some other way. If you have a package manager on the target system, you can just login on the system and install them.

Here are examples for Raspbian:

If ALSA support is enabled, install libasound:

.. code::

   $ apt-get install libasound2

If PulseAudio support is enabled, install libltdl and libpulse:

.. code::

   $ apt-get install libltdl7 libpulse0

.. _qemu:

Running cross-compiled tests in QEMU
====================================

Running a test on 64-bit ARMv8 CPU using `rocstreaming/toolchain-aarch64-linux-gnu <https://hub.docker.com/r/rocstreaming/toolchain-aarch64-linux-gnu/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4 \
          env LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/aarch64-linux-gnu/rpath" \
            qemu-aarch64 -L /opt/sysroot -cpu cortex-a53 \
              ./bin/aarch64-linux-gnu/roc-test-core

Running a test on 32-bit ARMv7 CPU using `rocstreaming/toolchain-arm-linux-gnueabihf <https://hub.docker.com/r/rocstreaming/toolchain-arm-linux-gnueabihf/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-linux-gnueabihf:gcc-4.9 \
          env LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/arm-linux-gnueabihf/rpath" \
            qemu-arm -L /opt/sysroot -cpu cortex-a15 \
              ./bin/arm-linux-gnueabihf/roc-test-core

Running a test on 32-bit ARMv6 CPU using `rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi <https://hub.docker.com/r/rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 \
          env LD_LIBRARY_PATH="/opt/sysroot/lib:${PWD}/3rdparty/arm-bcm2708hardfp-linux-gnueabi/rpath" \
            qemu-arm -L /opt/sysroot -cpu arm1176 \
              ./bin/arm-bcm2708hardfp-linux-gnueabi/roc-test-core
