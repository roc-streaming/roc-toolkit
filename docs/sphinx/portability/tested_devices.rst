Tested devices
**************

.. contents:: Table of contents:
   :local:
   :depth: 2

Overview
========

This page provides the list of devices used for acceptance testing of evey published release.

Testing includes building for target platform, running all unit and integration tests on hardware, and manual testing of sender and receiver. `QA repository <https://github.com/roc-streaming/qa/>`_ contains scripts that implements this procedure.

Linux single-board computers
============================

Raspberry Pi 4 Model B
----------------------

* Website: https://www.raspberrypi.com/products/raspberry-pi-4-model-b/
* OS: Raspberry Pi OS 32-bit, Ubuntu Server 22.04 64-bit
* Toolchain: :ref:`arm-linux-gnueabihf <arm-linux-gnueabihf>`, :ref:`aarch64-linux-gnu <aarch64-linux-gnu>`
* CPU: 64-bit ARMv8 Cortex-A72 Quad-core 1.5GHz
* RAM: 4GB
* Wi-Fi: 802.11ac

.. image:: https://roc-streaming.org/images/tested_devices/raspberry_pi_4_model_b.jpg
   :width: 300px

Raspberry Pi 3 Model B
----------------------

* Website: https://www.raspberrypi.org/products/raspberry-pi-3-model-b/
* OS: Raspbian Stretch 32-bit
* Toolchain: :ref:`arm-linux-gnueabihf <arm-linux-gnueabihf>`
* CPU: 64-bit ARMv8 Cortex-A53 Quad-core 1.2GHz
* RAM: 1GB
* Wi-Fi: 802.11n

.. image:: https://roc-streaming.org/images/tested_devices/raspberry_pi_3_model_b.jpg
   :width: 300px

Raspberry Pi Zero W
-------------------

* Website: https://www.raspberrypi.org/products/raspberry-pi-zero-w/
* OS: Raspbian Stretch 32-bit
* Toolchain: :ref:`arm-bcm2708hardfp-linux-gnueabi <arm-bcm2708hardfp-linux-gnueabi>`
* CPU: 32-bit ARMv6 BCM2835 Single-core 1GHz
* RAM: 512MB
* Wi-Fi: 802.11 b/g/n

.. image:: https://roc-streaming.org/images/tested_devices/raspberry_pi_zero_w.jpg
   :width: 300px

Orange Pi Lite 2
----------------

* Website: http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-Lite-2.html
* OS: Ubuntu 16.04 for OrangePi Lite2 64-bit
* Toolchain: :ref:`aarch64-linux-gnu <aarch64-linux-gnu>`
* CPU: 64-bit ARMv8 Cortex-A53 Quad-core 1.8GHz
* RAM: 1GB
* Wi-Fi: 802.11 ac/b/g/n

.. image:: https://roc-streaming.org/images/tested_devices/orange_pi_lite_2.jpg
   :width: 300px

OpenEmbed SOM9331 v1
--------------------

* Website: https://openwrt.org/toh/openembed/som9331_v1/
* OS: OpenWrt 17.01 32-bit
* Toolchain: :ref:`mips-openwrt-linux-atheros <mips-openwrt-linux-atheros>`
* CPU: MIPS32 24Kc Atheros AR9331 400 MHz
* RAM: 32MB
* Wi-Fi: 802.11 b/g/n

.. image:: https://roc-streaming.org/images/tested_devices/som9331_v1.jpg
   :width: 300px

Linux desktop computers
=======================

Dell XPS 15 9520
----------------

* Website: https://www.dell.com/en-us/shop/cty/pdp/spd/xps-15-9520-laptop/nbxn9520fyvlh
* OS: Debian 12.2 (bookworm)
* Toolchain: ``x86_64-pc-linux-gnu``
* CPU: Intel Core i9-12900HK 14-core 5 GHz
* RAM: 64GB
* Wi-Fi: 802.11 a/b/g/n/ac/ax

.. image:: https://roc-streaming.org/images/tested_devices/dell_xps_15.jpg
   :width: 300px

Apple desktop computers
=======================

MacBook Pro 2018
----------------

* Website: https://support.apple.com/kb/SP776?locale=en_US
* OS: macOS 12 (Monterey)
* Toolchain: ``x86_64-pc-apple-darwin21.6.0``
* CPU: Intel Core i7-8850H 6-core 2.6GHz
* RAM: 16GB
* Wi-Fi: 802.11ac

.. image:: https://roc-streaming.org/images/tested_devices/macbook_pro_2018.jpg
   :width: 300px

MacBook Air M1 2020
-------------------

* Website: https://www.apple.com/macbook-air-m1/
* OS: macOS 11 (Big Sur)
* Toolchain: ``arm64-pc-apple-darwin20.6.0``
* CPU: 64-bit ARMv8 8-core 3.2GHz+2.0GHz
* RAM: 8GB
* Wi-Fi: 802.11ax

.. image:: https://roc-streaming.org/images/tested_devices/macbook_air_m1_2020.jpg
   :width: 300px
