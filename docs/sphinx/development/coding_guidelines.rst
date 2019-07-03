Coding guidelines
*****************

.. contents:: Table of contents:
   :local:
   :depth: 1

Language and libraries
======================

* The codebase is mostly written in "C with classes", a simple and unsophisticated subset of C++.
* We use C++98.
* We don't use exceptions.
* We mostly avoid templates.
* We avoid inheritance, except "interface" inheritance.
* We avoid operators and overloading.
* We avoid using the preprocessor.
* We don't use STL (the algorithms-and-containers part of the C++ standard library).
* We don't use third-party utility libraries like Boost.
* We maintain our own minimal utility library (roc_core) designed to be very lightweight and allowing fine-grained memory management.

Portability
===========

* The code should run on a variety of operating systems, compilers, and hardware architectures, including rather old compilers and distributions. See :doc:`supported platforms </portability/supported_platforms>` page.

* The code specific to platform, compiler, or optional features and dependencies, should be isolated inside corresponding :ref:`target directories <targets>`. All other code should be portable across all supported configurations.

Coding conventions
==================

* Cover every component with class-level unit tests if possible. Additionally, cover high-level features with pipeline-level integration tests.

* Prefer RAII and smart pointers for resource management.

* Prefer either non-copyable or trivial-copy objects. Avoid making "heavy" operations, in particular, operations involving memory management, implicit.

* Use const when it is useful.

* Use allocators and pools for memory management.

* Log (using ``roc_log``) important events and information needed to understand why an error occurred.

* Panic (using ``roc_panic``) when a contract or an invariant is broken. A panic is always preferred over a crash. However, remember that panics are only for bugs in Roc itself. Never panic on invalid or unexpected data from the outside world.

Coding style
============

* The code should compile without warnings (use ``--enable-werror`` SCons option).

* The code should be formatted using ``scons fmt``, which invokes ``clang-format``. If it goes awry, you can prevent a file from being formatted by adding it to ``.fmtignore``.

* Header and source files should contain the "Roc authors" copyright and license header. Running ``scons fmt`` will automatically insert them.

* Headers, classes, public members, and free functions should be documented using Doxygen. If Doxygen is installed, it is invoked during build and warns about undocumented items.

* Prefer creating individual .h and .cpp files for every class. Use snake_case for file names and old-style header guards, which are automatically inserted by ``scons fmt``.

* Use CamelCase for class names, and snake_case for methods, functions, fields, and variables. Add trailing underscore\_ for private methods and fields.
