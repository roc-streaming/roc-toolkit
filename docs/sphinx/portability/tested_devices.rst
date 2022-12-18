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

.. image:: https://i.imgur.com/6RzCawM.jpg
   :width: 300px

Raspberry Pi 3 Model B
----------------------

* Website: https://www.raspberrypi.org/products/raspberry-pi-3-model-b/
* OS: Raspbian Stretch 32-bit
* Toolchain: :ref:`arm-linux-gnueabihf <arm-linux-gnueabihf>`
* CPU: 64-bit ARMv8 Cortex-A53 Quad-core 1.2GHz
* RAM: 1GB
* Wi-Fi: 802.11n

.. image:: https://i.imgur.com/lV5oA2w.jpg
   :width: 300px

Raspberry Pi Zero W
-------------------

* Website: https://www.raspberrypi.org/products/raspberry-pi-zero-w/
* OS: Raspbian Stretch 32-bit
* Toolchain: :ref:`arm-bcm2708hardfp-linux-gnueabi <arm-bcm2708hardfp-linux-gnueabi>`
* CPU: 32-bit ARMv6 BCM2835 Single-core 1GHz
* RAM: 512MB
* Wi-Fi: 802.11 b/g/n

.. image:: https://i.imgur.com/vtxdcim.jpg
   :width: 300px

Orange Pi Lite 2
----------------

* Website: http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-Lite-2.html
* OS: Ubuntu 16.04 for OrangePi Lite2 64-bit
* Toolchain: :ref:`aarch64-linux-gnu <aarch64-linux-gnu>`
* CPU: 64-bit ARMv8 Cortex-A53 Quad-core 1.8GHz
* RAM: 1GB
* Wi-Fi: 802.11 ac/b/g/n

.. image:: https://i.imgur.com/buarXAG.jpg
   :width: 300px

Linux desktop computers
=======================

HP ProBook 440 G4
-----------------

* Website: https://support.hp.com/us-en/product/hp-probook-440-g4-notebook-pc/10477248/document/c05273730
* OS: Debian 11.5 (bullseye)
* Toolchain: ``x86_64-pc-linux-gnu``
* CPU: Intel Core i7-7500U 2-core 2.7 GHz
* RAM: 16GB
* Wi-Fi: 802.11 b/g/n

.. image:: https://i.imgur.com/oNuTDJp.png
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

.. image:: https://i.imgur.com/urKO61D.jpg
   :width: 300px

MacBook Air M1 2020
-------------------

* Website: https://www.apple.com/macbook-air-m1/
* OS: macOS 11 (Big Sur)
* Toolchain: ``arm64-pc-apple-darwin20.6.0``
* CPU: 64-bit ARMv8 8-core 3.2GHz+2.0GHz
* RAM: 8GB
* Wi-Fi: 802.11ax

.. image:: https://i.imgur.com/eKX8G2j.jpg
   :width: 300px
