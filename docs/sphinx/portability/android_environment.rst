Android environment
*******************

.. contents:: Table of contents:
   :local:
   :depth: 1

Docker image
============

The ``rocstreaming/env-android`` image provide a full android environment for Android build and tests. It is used by ``android_emu.sh`` script described in :ref:`this section <android_docker>`, and in `roc-java <https://github.com/roc-streaming/roc-java/>`_ repo.

In particular, the following packages are availables:

* android platforms
* android build tools
* android ndk
* android cmake
* android emulator
* adb and platform tools

For reducing image size and have more granularity over various tools versions, those packages are installed only when container runs, i.e. at container entrypoint.

The following environment variables can be passed at container run for choosing a specified version:

* API
* BUILD_TOOLS_VERSION
* NDK_VERSION
* CMAKE_VERSION

Example:

.. code::

    $ docker run -t --rm -v "${PWD}:${PWD}" -w "${PWD}" -v android-sdk:/sdk --env API=28 \
      --env NDK_VERSION=21.1.6352462 --env BUILD_TOOLS_VERSION=29.0.3 \
        rocstreaming/env-android:jdk8 \
          scons -Q --compiler=clang --host=aarch64-linux-android28 \
            --disable-soversion \
            --disable-tools \
            --disable-examples \
            --disable-tests \
            --disable-pulseaudio \
            --disable-sox \
            --build-3rdparty=libuv,openfec

Tools caching
=============

If a named volume is mounted at `/sdk` path in the container (for example by using `-v android-sdk:/sdk` option), next run of the image will not install again components already installed previously.

If it's needed to mount the volume to a specific host location (the host location must exist) it can be achieved by adding the following options to the docker command:

.. code::

    --mount type=volume,dst=/sdk,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=<host-path>

Emulator
========

The android emulator can use hardware acceleration features to improve performance, sometimes drastically.

.. note::
  According to `official emulator acceleration docs <https://developer.android.com/studio/run/emulator-acceleration>`_:

  To use VM acceleration, your development environment must meet the following requirements:

    SDK Tools: minimum version 17; recommended version 26.1.1 or later
    AVD with an x86-based system image, available for Android 2.3.3 (API level 10) and higher

      Warning: AVDs that use ARM- or MIPS-based system images can't use the VM acceleration.

  In addition to the development environment requirements, your computer's processor must support one of the following virtualization extensions technologies:

    Intel Virtualization Technology (VT, VT-x, vmx) extensions
    AMD Virtualization (AMD-V, SVM) extensions

Linux-based systems support VM acceleration through the `KVM software package <https://www.linux-kvm.org/page/Main_Page>`_.

For enabling hardware acceleration run the container in privileged mode, i.e. by using ``--privileged`` flag.

.. warning::

  Since CI runs jobs already on a virtual environment, if the emulator need to be run on CI, the ``env-android`` image must be run with ``--privileged`` option for allowing virtualization nesting.

To see if acceleration is available use:

.. code::

    $ emulator -accel-check
    accel:
    0
    KVM (version 12) is installed and usable.

To create an Android Virtual Device (AVD) and run the emulator:

* download the emulator system image:

  .. code::

      $ yes | sdkmanager <system-image>

  where ``<system-image>`` is in the list offered by ``sdkmanager --list``

* create the AVD:

  .. code::

      $ echo no | avdmanager create avd --name <avd-name> --package <system-image>

* launch emulator (use ``-accel on`` or ``-accel off`` depending of hardware acceleration availability):

  .. code::

      $ emulator -avd <avd-name> -no-audio -no-boot-anim -no-window -gpu off -accel [on/off] &

* check the AVD status:

  .. code::

      $ adb devices
      List of devices attached
      emulator-xxxx device
      # "device" indicates that boot is completed
      # "offline" indicates that boot is still going on

Device script
=============

The ``env-android`` image provides an helper script named ``device`` that takes care of creating and booting up AVDs.

* create an AVD:

  .. code::

      $ device create --api=<API> --image=<IMAGE> --arch=<ARCH> --name=<AVD-NAME>

  The string ``"system-images;android-<API>;<IMAGE>;<ARCH>"`` defines the emulator system image to be installed (it must be present in the list offered by ``sdkmanager --list``)

* start device and wait until boot is completed

  .. code::

      $ device start --name=<AVD-NAME>
