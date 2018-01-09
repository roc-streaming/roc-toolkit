Continuous integration
**********************

Overview
========

Travis is configured to build ``master`` and ``develop`` branches and pull requests.

Travis builds Roc for Linux and MacOS. Linux worker uses Docker to run builds on several Linux distros. Linux worker also uses QEMU to run cross-compiled tests.

Docker images for continuous integration and cross-compilation are prepared using Docker Hub automated builds. They are based on official upstream images, adding pre-installed packages required for build. Dockerfiles for images are hosted in a separate GitHub repository. When the Dockerfile or the upstream image changes, Docker Hub automatically triggers rebuild.

Links:
 * `Roc on Travis <https://travis-ci.org/roc-project/>`_
 * `Roc on Docker Hub <https://hub.docker.com/u/rocproject/>`_
 * `Our Dockerfiles on GitHub <https://github.com/roc-project/docker-ci>`_
 * `Our Travis configuration <https://github.com/roc-project/roc/blob/master/.travis.yml>`_
 * `Our Travis scripts <https://github.com/roc-project/roc/tree/master/scripts/travis>`_

Docker builds
=============

============================= ================= ============= ==========
Image                         Base image        Architecture  Compilers
============================= ================= ============= ==========
rocproject/ci-ubuntu:17.04    ubuntu:17.04      x86_64        gcc-6, clang-3.9
rocproject/ci-ubuntu:16.04    ubuntu:16.04      x86_64        gcc-4.8, gcc-5, clang-3.7
rocproject/ci-ubuntu:14.04    ubuntu:14.04      x86_64        gcc-4.4, gcc-4.6, clang-3.4
rocproject/ci-fedora          fedora:latest     x86_64        distro default
rocproject/ci-debian          debian:stable     x86_64        distro default
rocproject/ci-centos          centos:latest     x86_64        distro default
rocproject/ci-empty           ubuntu:16.04      x86_64        distro default
rocproject/cross-raspberry    debian:stable     armv6         arm-bcm2708hardfp-linux-gnueabi-gcc-4.7
rocproject/cross-linaro       debian:stable     armv7         arm-linux-gnueabihf-gcc-4.9
============================= ================= ============= ==========

Run locally
===========

It is possible to run Linux builds locally, in the same environment as they have on Travis.

For example, this will run Fedora build:

.. code::

    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" \
        rocproject/ci-fedora \
          scons --with-3rdparty=openfec,cpputest variant=debug test
