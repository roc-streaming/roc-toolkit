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
        libuv1-dev libunwind-dev libatomic-ops-dev libpulse-dev libsox-dev libcpputest-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

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
        libunwind8-dev libatomic-ops-dev libpulse-dev libsox-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,openfec,cpputest

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,openfec,cpputest install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,openfec,pulseaudio,cpputest

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,openfec,pulseaudio,cpputest install

Fedora 22 and later
-------------------

.. code::

    # for Roc
    $ sudo dnf install gcc-c++ pkgconfig scons ragel gengetopt \
        libuv-devel libunwind-devel libatomic_ops-devel pulseaudio-libs-devel sox-devel

    # for 3rd-parties
    $ sudo dnf install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec,cpputest

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec,cpputest install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest install

Centos 7 and later
------------------

.. code::

    # for developer packages
    $ sudo yum install epel-release

    # for Roc
    $ sudo yum install gcc-c++ pkgconfig scons ragel gengetopt \
        libunwind-devel libatomic_ops-devel pulseaudio-libs-devel sox-devel

    # for 3rd-parties
    $ sudo yum install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,openfec,cpputest

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,openfec,cpputest install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,openfec,pulseaudio,cpputest

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=libuv,openfec,pulseaudio,cpputest install

openSUSE Leap and later
-----------------------

.. code::

    # for Roc
    $ sudo zypper install gcc-c++ scons ragel gengetopt \
         libuv-devel libunwind-devel libatomic_ops-devel libpulse-devel sox-devel

    # for 3rd-parties
    $ sudo zypper install pkg-config intltool libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec,cpputest

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec,cpputest install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest install

Arch Linux
----------

.. code::

    # for Roc
    $ sudo pacman -S gcc pkgconf scons ragel gengetopt libuv libunwind libatomic_ops libpulse sox

    # for 3rd-parties
    $ sudo pacman -S grep gawk libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec,cpputest

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec,cpputest install

    # build libraries, tools, and PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest

    # install libraries, tools, and PulseAudio modules
    $ sudo scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest install

Alpine Linux
------------

.. code::

    # for Roc
    $ sudo apk add g++ pkgconf scons ragel gengetopt \
        libuv-dev libunwind-dev libatomic_ops-dev pulseaudio-dev sox-dev cpputest

    # for 3rd-parties
    $ sudo apk add libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

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
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:12.2,sox,cpputest

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
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-bcm2708hardfp-linux-gnueabi \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-bcm2708hardfp-linux-gnueabi \
            --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:5.0,sox,cpputest

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
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-aarch64-linux-gnu \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=aarch64-linux-gnu \
            --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:8.0,sox,cpputest

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
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries, tools, and PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=libuv,libunwind,libatomic_ops,openfec,alsa,pulseaudio:8.0,sox,cpputest

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
    $ brew install scons ragel gengetopt sox libuv libatomic_ops cpputest

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=openfec

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=openfec install

macOS 10.11 and later
---------------------

.. code::

    # for Roc
    $ brew install scons ragel gengetopt libatomic_ops cpputest

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # build libraries and tools
    $ scons -Q --build-3rdparty=libuv,openfec,sox

    # install libraries and tools
    $ sudo scons -Q --build-3rdparty=libuv,openfec,sox install

Android
=======

.. warning::

   Android support is still work in progress and was not properly tested yet.

.. seealso::

   * :doc:`/portability/cross_compiling`

Termux packages
---------------

.. warning::

   Termux package for Roc may be outdated.

Install `Termux <https://termux.com/>`_ on your device and enter these commands:

.. code::

    $ pkg install unstable-repo
    $ pkg install roc
    $ pkg install pulseaudio

This will install binary packages for PulseAudio daemon and Roc PulseAudio modules on your device. Then you can configure PulseAudio to run Roc as described in :doc:`/running/pulseaudio_modules`.
