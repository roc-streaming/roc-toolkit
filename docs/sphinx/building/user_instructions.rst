User instructions
*****************

.. contents:: Jump to:
   :local:
   :depth: 1

Linux
=====

Ubuntu 15.10 and later
----------------------

.. code::

    # for Roc
    $ apt-get install g++ pkg-config scons gengetopt libuv1-dev libsox-dev libcpputest-dev

    # for 3rd-parties
    $ apt-get install libtool intltool autoconf automake make cmake

    # libraries, tools, tests
    $ scons --build-3rdparty=openfec test

    # PulseAudio modules
    $ scons --enable-pulseaudio-modules --build-3rdparty=openfec,pulseaudio

Ubuntu 14.04 and later, Debian Jessie and later
-----------------------------------------------

.. code::

    # for Roc
    $ apt-get install g++ pkg-config scons gengetopt libsox-dev

    # for 3rd-parties
    $ apt-get install libtool intltool autoconf automake make cmake

    # libraries, tools, tests
    $ scons --build-3rdparty=uv,openfec,cpputest test

    # PulseAudio modules
    $ scons --enable-pulseaudio-modules --build-3rdparty=uv,openfec,cpputest,pulseaudio

Fedora 22 and later
-------------------

.. code::

    # for Roc
    $ dnf install gcc-c++ pkgconfig scons gengetopt libuv-devel sox-devel

    # for 3rd-parties
    $ dnf install libtool intltool autoconf automake make cmake

    # libraries, tools, tests
    $ scons --build-3rdparty=openfec,cpputest test

    # PulseAudio modules
    $ scons --enable-pulseaudio-modules --build-3rdparty=openfec,cpputest,pulseaudio

Centos 7 and later
------------------

.. code::

    # for developer packages
    $ yum install epel-release

    # for Roc
    $ yum install gcc-c++ pkgconfig scons gengetopt sox-devel

    # for 3rd-parties
    $ yum install libtool intltool autoconf automake make cmake

    # libraries, tools, tests
    $ scons --build-3rdparty=uv,openfec,cpputest test

    # PulseAudio modules
    $ scons --enable-pulseaudio-modules --build-3rdparty=uv,openfec,cpputest,pulseaudio

MacOS
=====

.. code::

    # for Roc
    $ brew install scons gengetopt sox libuv

    # for 3rd-parties
    $ brew install libtool autoconf automake make cmake

    # libraries, tools, tests
    $ scons --build-3rdparty=openfec,cpputest test
