User cookbook
*************

.. contents:: Table of contents:
   :local:
   :depth: 2

Linux (from packages)
=====================

Ubuntu / Debian
---------------

Download debian packages from `latest release <https://github.com/roc-streaming/roc-toolkit/releases/latest>`_, go to the download directory, and run:

.. code::

    $ sudo apt install ./roc_*.deb ./libroc_*.deb ./libroc-dev_*.deb

Fedora
------

.. code::

    $ sudo dnf install roc-toolkit roc-toolkit-utils roc-toolkit-devel

Arch Linux
----------

.. code::

    $ sudo pacman -S roc-toolkit libpulse sox

Alpine Linux
------------

.. code::

    $ sudo apk add roc-toolkit roc-toolkit-libs roc-toolkit-dev

Linux (from sources)
====================

Ubuntu / Debian
---------------

.. code::

    # for Roc
    $ sudo apt-get install g++ pkg-config scons ragel gengetopt \
        libuv1-dev libunwind-dev libspeexdsp-dev libsox-dev libssl-dev libpulse-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake meson

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

Fedora
------

.. code::

    # for Roc
    $ sudo dnf install gcc-c++ pkgconfig scons ragel gengetopt \
        libuv-devel libunwind-devel speexdsp-devel sox-devel openssl-devel pulseaudio-libs-devel

    # for 3rd-parties
    $ sudo dnf install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

openSUSE
--------

.. code::

    # for Roc
    $ sudo zypper install gcc-c++ scons ragel gengetopt \
        libuv-devel libunwind-devel speexdsp-devel sox-devel libopenssl-3-devel libpulse-devel

    # for 3rd-parties
    $ sudo zypper install pkg-config intltool libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

Centos
------

.. code::

    # for developer packages
    $ sudo yum install epel-release

    # for Roc
    $ sudo yum install gcc-c++ pkgconfig scons ragel gengetopt \
        libunwind-devel speex-devel sox-devel openssl11-devel pulseaudio-libs-devel

    # for 3rd-parties
    $ sudo yum install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,libatomic_ops,openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,libatomic_ops,openfec install

Arch Linux
----------

.. code::

    # for Roc
    $ sudo pacman -S gcc pkgconf scons ragel gengetopt \
        libuv libunwind speexdsp sox openssl gsm libpulse

    # for 3rd-parties
    $ sudo pacman -S grep gawk libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

Alpine Linux
------------

.. code::

    # for Roc
    $ sudo apk add g++ pkgconf scons ragel gengetopt \
        libuv-dev libunwind-dev speexdsp-dev sox-dev openssl-dev pulseaudio-dev

    # for 3rd-parties
    $ sudo apk add libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

NixOS
-----

.. code::

    # for Roc and 3rd-parties
    $ nix-shell -p gcc autoconf automake cmake gengetopt gnumake intltool libpulseaudio \
        libtool libunwind libuv meson pkg-config ragel scons sox openssl speexdsp

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

Linux (cross-compile)
=====================

.. seealso::

   * :doc:`/portability/cross_compiling`
   * :doc:`/portability/tested_devices`

Raspberry Pi (64-bit)
---------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-aarch64-linux-gnu:gcc-7.4 \
          scons -Q \
            --host=aarch64-linux-gnu \
            --build-3rdparty=all \
            --disable-pulseaudio

    # install Roc binaries
    $ scp ./bin/aarch64-linux-gnu/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/aarch64-linux-gnu/libroc.so.*.* <address>:/usr/lib

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

Raspberry Pi 2 and later (32-bit)
---------------------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-linux-gnueabihf:gcc-4.9 \
          scons -Q \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=all \
            --disable-pulseaudio

    # install Roc binaries
    $ scp ./bin/arm-linux-gnueabihf/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-linux-gnueabihf/libroc.so.*.* <address>:/usr/lib

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

Raspberry Pi 1 and Zero (32-bit)
--------------------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi:gcc-4.7 \
          scons -Q \
            --host=arm-bcm2708hardfp-linux-gnueabi \
            --build-3rdparty=all

    # install Roc binaries
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/libroc.so.*.* <address>:/usr/lib

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

macOS
=====

Prerequisites:

* Install XCode Command Line Tools
* Install `Homebrew <https://brew.sh/>`_

Then you can run the following commands:

.. code::

    # for Roc
    $ brew install pkg-config scons ragel gengetopt libuv speexdsp sox openssl@3

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

Android
=======

.. seealso::

   * `Roc Droid <https://github.com/roc-streaming/roc-droid>`_ (android app)
   * `Roc Java <https://github.com/roc-streaming/roc-java>`_ (JAR and AAR shipped with precompiled libroc)
   * :doc:`/portability/cross_compiling`

Building C library for Android using Docker
-------------------------------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libroc.so for 64-bit ARM, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=aarch64-linux-android28 \
            --build-3rdparty=all

    # build libroc.so for 32-bit ARM, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=armv7a-linux-androideabi28 \
            --build-3rdparty=all

    # build libroc.so for 64-bit Intel, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=x86_64-linux-android28 \
            --build-3rdparty=all

    # build libroc.so for 32-bit Intel, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=i686-linux-android28 \
            --build-3rdparty=all

Building C library for Android on macOS
---------------------------------------

Prerequisites:

* Install `Android SDK command-line tools <https://github.com/codepath/android_guides/wiki/Installing-Android-SDK-Tools>`_, in particular ``sdkmanager``.

* Ensure that ``sdkmanager`` is in ``PATH`` and working.

* Ensure that ``ANDROID_HOME`` is exported and points to the root directory of Android SDK.

Then you can run the following commands:

.. code::

    # install Android components (you can use higher versions)
    $ sdkmanager 'platforms;android-29'
    $ sdkmanager 'build-tools;28.0.3'
    $ sdkmanager 'ndk;21.4.7075529'
    $ sdkmanager 'cmake;3.10.2.4988404'

    # install build tools
    $ brew install scons ragel gengetopt

    # add toolchains to PATH
    $ export PATH="$ANDROID_HOME/ndk/21.4.7075529/toolchains/llvm/prebuilt/darwin-x86_64/bin:$PATH"

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libroc.so for 64-bit ARM, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=all \
          --compiler=clang \
          --host=aarch64-linux-android29

    # build libroc.so for 32-bit ARM, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=all \
          --compiler=clang \
          --host=armv7a-linux-androideabi24

    # build libroc.so for 64-bit Intel, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=all \
          --compiler=clang \
          --host=x86_64-linux-android29

    # build libroc.so for 32-bit Intel, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=all \
          --compiler=clang \
          --host=i686-linux-android29
