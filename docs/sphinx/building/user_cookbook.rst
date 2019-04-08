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
    $ sudo apt-get install g++ pkg-config scons gengetopt libuv1-dev libsox-dev libcpputest-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=openfec test

    # PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

Ubuntu 14.04 and later, Debian Jessie and later
-----------------------------------------------

.. code::

    # for Roc
    $ sudo apt-get install g++ pkg-config scons gengetopt libsox-dev

    # for 3rd-parties
    $ sudo apt-get install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=uv,openfec,cpputest test

    # PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=uv,openfec,pulseaudio,cpputest

Fedora 22 and later
-------------------

.. code::

    # for Roc
    $ sudo dnf install gcc-c++ pkgconfig scons gengetopt libuv-devel sox-devel

    # for 3rd-parties
    $ sudo dnf install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=openfec,cpputest test

    # PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio,cpputest

Centos 7 and later
------------------

.. code::

    # for developer packages
    $ sudo yum install epel-release

    # for Roc
    $ sudo yum install gcc-c++ pkgconfig scons gengetopt sox-devel

    # for 3rd-parties
    $ sudo yum install libtool intltool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=uv,openfec,cpputest test

    # PulseAudio modules
    $ scons -Q --enable-pulseaudio-modules --build-3rdparty=uv,openfec,pulseaudio,cpputest

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

    # libraries, tools, PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=uv,openfec,alsa,pulseaudio:10.0,sox,cpputest

    # install
    $ scp ./bin/arm-linux-gnueabihf/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-linux-gnueabihf/libroc.so /usr/lib
    $ scp ./bin/arm-linux-gnueabihf/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-10.0/modules

Raspberry Pi 1 and Zero
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-bcm2708hardfp-linux-gnueabi \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-bcm2708hardfp-linux-gnueabi \
            --build-3rdparty=uv,openfec,alsa,pulseaudio:5.0,sox,cpputest

    # install
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/libroc.so /usr/lib
    $ scp ./bin/arm-bcm2708hardfp-linux-gnueabi/module-roc-{sink,sink-input} \
        <address>:/usr/lib/pulse-5.0/modules

Orange Pi 64-bit models
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-aarch64-linux-gnu \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=aarch64-linux-gnu \
            --build-3rdparty=uv,openfec,alsa,pulseaudio:8.0,sox,cpputest

    # install
    $ scp ./bin/aarch64-linux-gnu/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/aarch64-linux-gnu/libroc.so /usr/lib
    $ scp ./bin/aarch64-linux-gnu/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-8.0/modules

Orange Pi 32-bit models
-----------------------

.. code::

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, PulseAudio modules
    $ docker run -t --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/cross-arm-linux-gnueabihf \
          scons -Q \
            --enable-pulseaudio-modules \
            --host=arm-linux-gnueabihf \
            --build-3rdparty=uv,openfec,alsa,pulseaudio:8.0,sox,cpputest

    # install
    $ scp ./bin/arm-linux-gnueabihf/roc-{recv,send,conv} <address>:/usr/bin
    $ scp ./bin/arm-linux-gnueabihf/libroc.so /usr/lib
    $ scp ./bin/arm-linux-gnueabihf/module-roc-{sink,sink-input} <address>:/usr/lib/pulse-8.0/modules

macOS
=====

macOS 10.12 and later
---------------------

.. code::

    # for Roc
    $ brew install scons gengetopt sox libuv

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=openfec,cpputest test

macOS 10.11 and later
---------------------

.. code::

    # for Roc
    $ brew install scons gengetopt

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # clone repo
    $ git clone https://github.com/roc-project/roc.git
    $ cd roc

    # libraries, tools, tests
    $ scons -Q --build-3rdparty=uv,openfec,sox,cpputest test
