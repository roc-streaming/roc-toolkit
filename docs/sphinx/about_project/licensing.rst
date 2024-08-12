Licensing
*********

Roc Toolkit source code is licensed under `MPL-2.0 <https://www.mozilla.org/en-US/MPL/2.0/>`_.

You may also want to check licenses of the :doc:`dependencies </building/dependencies>`. If you can't use dependencies with strong copyleft licenses like GPL, you may need to disable them at build time.

OpenFEC
=======

OpenFEC is an optional but very useful dependency that allows to restore lost packets. OpenFEC source code has several licenses. Most of the code is licensed under CeCCIL-C, which is LGPL-like. LDPC-Staircase codec, however, is licensed under CeCCIL, a stricter GPL-like license. There are also portions under BSD-like and CC BY-SA licenses that may require attribution.

If you can't fulfill CeCCIL requirements, you can build OpenFEC with LDPC-Staircase disabled. When using `our fork <https://github.com/roc-streaming/openfec>`_, you can pass ``-DOF_USE_LDPC_STAIRCASE_CODEC=OFF`` option to cmake. Alternatively, you can disable OpenFEC dependency altogether by passing ``--disable-openfec`` option to scons, but then you won't have loss recovery.
