Licensing
*********

Source code
===========

Roc Toolkit source code is licensed under `MPL-2.0 <https://www.mozilla.org/en-US/MPL/2.0/>`_.

Included third-party code
~~~~~~~~~~~~~~~~~~~~~~~~~

The following third-party code is included in Roc Toolkit source tree:

* dr_wav (MIT-0)
* hedley (CC0)
* Lock-Free free list implementation by Cameron Desrochers (BSD 2-Clause)

Each component retains its original license notice within its source files. The complete Roc Toolkit distribution is available under MPL-2.0, as all included third-party code is compatible with this license.

Dependencies
============

Roc Toolkit depends on several external libraries. The effective license of your final application will depend on which dependencies you enable during build.

Review the :doc:`dependencies page </building/dependencies>` for detailed license information. You may want to disable dependencies with strong copyleft licenses (such as GPL) at build time to maintain license compatibility with your project.

OpenFEC
~~~~~~~

OpenFEC is an optional but very useful dependency that allows to restore lost packets. OpenFEC source code has several licenses. Most of the code is licensed under CeCCIL-C, which is LGPL-like. LDPC-Staircase codec, however, is licensed under CeCCIL, a stricter GPL-like license. There are also portions under BSD-like and CC BY-SA licenses that may require attribution.

If you can't fulfill CeCCIL requirements, you can build OpenFEC with LDPC-Staircase disabled. When using `our fork <https://github.com/roc-streaming/openfec>`_, you can pass ``-DOF_USE_LDPC_STAIRCASE_CODEC=OFF`` option to cmake. Alternatively, you can disable OpenFEC dependency altogether by passing ``--disable-openfec`` option to scons, but then you won't have loss recovery.
