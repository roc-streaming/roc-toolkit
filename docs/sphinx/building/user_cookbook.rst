User cookbook
*************

.. contents:: Table of contents:
   :local:
   :depth: 2

Linux (native)
==============

Ubuntu 16.04 and later
----------------------

.. code::

    # for Roc
    $ sudo apt-get install g++ pkg-config scons ragel gengetopt \
        libuv1-dev libunwind-dev libspeexdsp-dev libsox-dev libpulse-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio install

Ubuntu 14.04 and later, Debian Jessie and later
-----------------------------------------------

.. code::

    # for Roc
    $ sudo apt-get install g++ pkg-config scons ragel gengetopt \
        libunwind8-dev libspeexdsp-dev libsox-dev libpulse-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,libatomic_ops,openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,libatomic_ops,openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio install

Fedora 22 and later
-------------------

.. code::

    # for Roc
    $ sudo dnf install gcc-c++ pkgconfig scons ragel gengetopt \
        libuv-devel libunwind-devel speexdsp-devel sox-devel pulseaudio-libs-devel

    # for 3rd-parties
    $ sudo dnf install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio install

Centos 7 and later
------------------

.. code::

    # for developer packages
    $ sudo yum install epel-release

    # for Roc
    $ sudo yum install gcc-c++ pkgconfig scons ragel gengetopt \
        libunwind-devel speex-devel sox-devel pulseaudio-libs-devel

    # for 3rd-parties
    $ sudo yum install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,libatomic_ops,openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,libatomic_ops,openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,libatomic_ops,openfec,pulseaudio install

openSUSE Leap and later
-----------------------

.. code::

    # for Roc
    $ sudo zypper install gcc-c++ scons ragel gengetopt \
         libuv-devel libunwind-devel speexdsp-devel sox-devel libpulse-devel

    # for 3rd-parties
    $ sudo zypper install pkg-config intltool libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio install

Arch Linux
----------

.. code::

    # for Roc
    $ sudo pacman -S gcc pkgconf scons ragel gengetopt libuv libunwind speexdsp sox libpulse

    # for 3rd-parties
    $ sudo pacman -S grep gawk libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio install

Alpine Linux
------------

.. code::

    # for Roc
    $ sudo apk add g++ pkgconf scons ragel gengetopt \
        libuv-dev libunwind-dev speexdsp-dev sox-dev pulseaudio-dev

    # for 3rd-parties
    $ sudo apk add libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio install

Linux (cross-compile)
=====================

.. seealso::

   * :doc:`/portability/cross_compiling`
   * :doc:`/portability/tested_boards`

Raspberry Pi 2 and 3
--------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:12.2,speexdsp,sox

    # install Roc binaries
    $ scp ./bin/arm-linux-gnueabihf/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-linux-gnueabihf/libroc.so.*.* <address>:/usr/lib
    $ scp ./bin/arm-linux-gnueabihf/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-12.2/modules

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

Raspberry Pi 1 and Zero
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-bcm2708hardfp-linux-gnueabi \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-bcm2708hardfp-linux-gnueabi \
            --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:5.0,speexdsp,sox

    # install Roc binaries
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/libroc.so.*.* <address>:/usr/lib
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/module-roc-{sink,sink-input} \
        <address>:/usr/lib/pulse-5.0/modules

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

Orange Pi 64-bit models
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-aarch64-linux-gnu \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=aarch64-linux-gnu \
            --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:8.0,speexdsp,sox

    # install Roc binaries
    $ scp ./bin/aarch64-linux-gnu/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/aarch64-linux-gnu/libroc.so.*.* <address>:/usr/lib
    $ scp ./bin/aarch64-linux-gnu/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-8.0/modules

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

Orange Pi 32-bit models
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=libuv,libunwind,openfec,alsa,pulseaudio:8.0,speexdsp,sox

    # install Roc binaries
    $ scp ./bin/arm-linux-gnueabihf/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-linux-gnueabihf/libroc.so.*.* <address>:/usr/lib
    $ scp ./bin/arm-linux-gnueabihf/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-8.0/modules

    # install Roc dependencies
    $ ssh <address> apt-get install libasound2 libpulse0 libltdl7

macOS
=====

macOS 10.12 and later
---------------------

.. code::

    # for Roc
    $ brew install scons ragel gengetopt libuv speexdsp sox

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

macOS 10.11 and later
---------------------

.. code::

    # for Roc
    $ brew install scons ragel gengetopt speexdsp

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-streaming/roc-toolkit.git
    $ cd roc-toolkit

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,openfec,sox

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,openfec,sox install

Android
=======

.. seealso::

   * `Roc Android app <https://github.com/roc-streaming/roc-droid>`_ (for end-users)
   * `Roc Java bindings <https://github.com/roc-streaming/roc-java>`_ (for Java/Kotlin developers; shipped with precompiled C library)
   * :doc:`/portability/cross_compiling`

Using Termux packages on Android
--------------------------------

.. warning::

   Termux package for Roc may be outdated.

Install `Termux <https://termux.com/>`_ on your Android device and enter these commands:

.. code::

    $ pkg install unstable-repo
    $ pkg install roc
    $ pkg install pulseaudio

This will install binary packages for PulseAudio daemon and Roc PulseAudio modules on your device. Then you can configure PulseAudio to run Roc as described in :doc:`/running/pulseaudio_modules`.

Building C library for Android on Linux
---------------------------------------

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
            --build-3rdparty=libuv,openfec,speexdsp

    # build libroc.so for 32-bit ARM, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=armv7a-linux-androideabi28 \
            --build-3rdparty=libuv,openfec,speexdsp

    # build libroc.so for 64-bit Intel, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=x86_64-linux-android28 \
            --build-3rdparty=libuv,openfec,speexdsp

    # build libroc.so for 32-bit Intel, API level 28
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocstreaming/toolchain-linux-android \
          scons -Q \
            --disable-tools \
            --compiler=clang \
            --host=i686-linux-android28 \
            --build-3rdparty=libuv,openfec,speexdsp

Building C library for Android on macOS
---------------------------------------

Prerequisites:

* Install `Android SDK command-line tools <https://github.com/codepath/android_guides/wiki/Installing-Android-SDK-Tools>`_, in particlar ``sdkmanager``.

* Ensure that ``sdkmanager`` is in ``PATH`` and working.

* Ensure that ``ANDROID_HOME`` is exported and points to the root directory of Android SDK.

Then you can run the following commands:

.. code::

    # install Android components (you can use higher versions)
    $ sdkmanager 'platforms;android-24'
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
          --build-3rdparty=libuv,openfec,speexdsp \
          --compiler=clang \
          --host=aarch64-linux-android24

    # build libroc.so for 32-bit ARM, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=libuv,openfec,speexdsp \
          --compiler=clang \
          --host=armv7a-linux-androideabi24

    # build libroc.so for 64-bit Intel, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=libuv,openfec,speexdsp \
          --compiler=clang \
          --host=x86_64-linux-android24

    # build libroc.so for 32-bit Intel, API level 24
    $ scons -Q \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=libuv,openfec,speexdsp \
          --compiler=clang \
          --host=i686-linux-android24
