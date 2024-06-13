Supported platforms
*******************

.. seealso::

   * :doc:`/development/continuous_integration`
   * :doc:`/portability/tested_devices`

What is supported
-----------------

The following platforms are officially supported.

Operating systems:

* GNU/Linux
* macOS
* Android

Compilers:

* GCC (>= 4.4)
* Clang (>= 3.4)

Libc:

* glibc
* musl
* uClibc
* macOS libSystem
* Bionic

What is tested
--------------

The following specific platform configurations are tested on a regular basis, partially :doc:`on CI </development/continuous_integration>`, and partially :doc:`on hardware </portability/tested_devices>`.

Linux distributions:

* Ubuntu (24.04, 22.04, 20.04, 18.04, 16.04, 14.04)
* Debian (stable)
* Fedora (latest)
* CentOS (latest)
* openSUSE (Leap)
* Arch Linux (latest)
* Alpine Linux (latest)
* Raspberry Pi OS (latest)
* OpenWrt (17.01, 12.09)

Android versions:

* ABI: arm64-v8a (ARM 64-bit), armeabi-v7a (ARM 32-bit), x86_64 (Intel 64-bit), x86 (Intel 32-bit)
* API: 29 (Android 10), 24 (Android 7)
* NDK: 21

macOS versions:

* macOS 14 (Sonoma)
* macOS 13 (Ventura)
* macOS 12 (Monterey)

Hardware architectures:

* x86_64
* i686
* ARMv8-A (Cortex-A 64-bit, Apple M1)
* ARMv7-A (Cortex-A 32-bit)
* ARMv6
* MIPS32 24K
