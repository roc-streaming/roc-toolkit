Cross compiling
***************

.. contents:: Jump to:
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

Use ``--host`` option to specify the toolchain name. This option defines a prefix which is added to all build tools, which should be available in PATH. For example, ``--host=arm-bcm2708hardfp-linux-gnueabi`` means that Roc will expect that ``arm-bcm2708hardfp-linux-gnueabi-gcc``, ``arm-bcm2708hardfp-linux-gnueabi-ld``, etc. are available in PATH.

Use ``--build-3rdparty`` option to let Roc to download and build dependencies. When it is used with ``--host``, Roc automatically uses the specified toolchain for dependencies.

If necessary, you can override build tool names and options by setting variables like ``CC`` and ``CFLAGS``.

See :doc:`/building/scons` page for the full list of options and variables.

Raspberry Pi toolchains (Raspberry Pi 1 and later)
==================================================

The official Raspberry Pi `tools <https://github.com/raspberrypi/tools>`_ repository contains several arm-bcm2708 prebuilt toolchains. BCM-2708 is a chip family which includes BCM-2835, BCM-2836, and BCM-2837 chips used in various Raspberry Pi models (see `RPi Hardware <https://elinux.org/RPi_Hardware>`_).

The ``arm-bcm2708hardfp-linux-gnueabi`` toolchain can be used for all Raspberry Pi models and likely Raspberry Pi clones too. It supports ARMv6 architecture, but not ARMv7 and ARMv8. Since all these architectures are backward-compatible, this toolchain can be used with all of them, but it can't employ ARMv7 or ARMv8 specific instructions.

Here is how you can build Roc with this toolchain using our `rocproject/cross-raspberry <https://hub.docker.com/r/rocproject/cross-raspberry/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
      rocproject/cross-raspberry \
      bash -c '
        PATH="/opt/toolchains/arm-bcm2708hardfp-linux-gnueabi/bin:${PATH}"
          scons -Q --build-3rdparty=uv,openfec,sox,cpputest \
            --host=arm-bcm2708hardfp-linux-gnueabi'

Or you can install the toolchain manually:

.. code::

    # setup directories
    $ RPI_TOOLS_DIR=/path/to/toolchain
    $ ROC_DIR=/path/to/roc

    # for Roc
    $ apt-get install g++ scons gengetopt

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
    $ scons --host=arm-bcm2708hardfp-linux-gnueabi --build-3rdparty=uv,openfec,sox,cpputest

Linaro toolchains (Raspberry Pi 2 and later, Orange Pi, Banana Pi)
==================================================================

The Linaro project provides several `toolchains <https://www.linaro.org/downloads/>`_ for different architectures and GCC versions.

The ``arm-linux-gnueabihf`` toolchain can be used Raspberry Pi 2 and later, Orange Pi, Banana Pi, and other boards compatible with ARMv7. It will not work for Raspberry Pi 1, which uses ARMv6.

Here is how you can build Roc with this toolchain using our `rocproject/cross-linaro <https://hub.docker.com/r/rocproject/cross-linaro/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
      rocproject/cross-linaro \
      bash -c '
        PATH="/opt/toolchains/arm-linux-gnueabihf/bin:${PATH}"
          scons -Q --build-3rdparty=uv,openfec,sox,cpputest \
            --host=arm-linux-gnueabihf'

Or you can install the toolchain manually:

.. code::

    # setup directories
    $ TOOLCHAIN_DIR=/path/to/toolchain
    $ ROC_DIR=/path/to/roc

    # for Roc
    $ apt-get install g++ scons gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # download toolchain
    $ wget http://releases.linaro.org/components/toolchain/binaries/4.9-2016.02/arm-linux-gnueabihf/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf.tar.xz
    $ tar -C "${TOOLCHAIN_DIR}" -Jf gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf.tar.xz
    $ export PATH="${TOOLCHAIN_DIR}/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf/bin:${PATH}"

    # build Roc
    $ cd "${ROC_DIR}"
    $ scons --host=arm-linux-gnueabihf --build-3rdparty=uv,openfec,sox,cpputest

Debian and Ubuntu toolchains
============================

Debian and Ubuntu provide packaged toolchains as well, see `CrossToolchains <https://wiki.debian.org/CrossToolchains>`_ page on Debian wiki.

The ``arm-linux-gnueabihf`` toolchain may be used to cross-compile binaries for ARMv7. However note that the resulting binary will require recent Glibc and probably won't run on Raspbian versions which don't have one.

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
    $ apt-get install g++ scons gengetopt

    # for 3rd-parties
    $ apt-get install libtool autoconf automake make cmake

    # install toolchain
    $ apt-get install crossbuild-essential-armhf

    # build Roc
    $ cd /path/to/roc
    $ scons --host=arm-linux-gnueabihf --build-3rdparty=uv,openfec,sox,cpputest

Running ARM executables in QEMU
===============================

Running an executable on ARMv6 CPU using our `rocproject/cross-raspberry <https://hub.docker.com/r/rocproject/cross-raspberry/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
      rocproject/cross-raspberry \
      bash -c '
        EXECUTABLE="roc-test-core"; \
        CPU="arm1176"; \
        TOOLCHAIN="arm-bcm2708hardfp-linux-gnueabi"; \
        SYSROOT="/opt/toolchains/${TOOLCHAIN}/${TOOLCHAIN}"; \
        LD_LIBRARY_PATH="${SYSROOT}/lib:${PWD}/3rdparty/${TOOLCHAIN}/lib" \
          qemu-arm -L "${SYSROOT}" -cpu "${CPU}" \
            ./bin/${TOOLCHAIN}/${EXECUTABLE}'

Running an executable on ARMv7 CPU using our `rocproject/cross-linaro <https://hub.docker.com/r/rocproject/cross-linaro/>`_ Docker image:

.. code::

    $ cd /path/to/roc
    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
      rocproject/cross-linaro \
      bash -c '
        EXECUTABLE="roc-test-core"; \
        CPU="cortex-a15"; \
        TOOLCHAIN="arm-linux-gnueabihf"; \
        SYSROOT="/opt/toolchains/${TOOLCHAIN}/${TOOLCHAIN}"; \
        LD_LIBRARY_PATH="${SYSROOT}/lib:${PWD}/3rdparty/${TOOLCHAIN}/lib" \
          qemu-arm -L "${SYSROOT}" -cpu "${CPU}" \
            ./bin/${TOOLCHAIN}/${EXECUTABLE}'
