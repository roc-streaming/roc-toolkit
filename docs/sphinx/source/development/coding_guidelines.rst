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
* We avoid operators, overloading, and default arguments.
* We avoid using the preprocessor.
* We don't use STL (the algorithms-and-containers part of the C++ standard library).
* We don't use third-party utility libraries like Boost.
* We maintain our own minimal utility library (roc_core) designed to be very lightweight and allowing fine-grained memory management.

Portability
===========

* The code should run on a variety of operating systems, compilers, and hardware architectures, including rather old compilers and distributions. See :doc:`supported platforms </portability/supported_platforms>` page.

.. raw:: html

    <span></span>

* The code specific to platform, compiler, or optional features and dependencies, should be isolated inside corresponding :ref:`target directories <targets>`. All other code should be portable across all supported configurations.

Best practices
==============

* The code should compile without warnings (use ``--enable-werror`` SCons option).

.. raw:: html

    <span></span>

* Cover every component with class-level unit tests if possible. Additionally, cover high-level features with pipeline-level integration tests. We use `CppUTest <https://cpputest.github.io/>`_.

.. raw:: html

    <span></span>

* Prefer RAII and smart pointers for resource management.

.. raw:: html

    <span></span>

* Prefer either non-copyable or trivial-copy objects. Avoid making "heavy" operations implicit, in particular, operations involving memory management.

.. raw:: html

    <span></span>

* Use const when it is useful.

.. raw:: html

    <span></span>

* Use anonymous namespaces instead of static globals, functions, and constants.

.. raw:: html

    <span></span>

* Use enums instead of defines, when possible.

.. raw:: html

    <span></span>

* Use allocators and pools for memory management.

.. raw:: html

    <span></span>

* Log (using ``roc_log``) important events and information needed to understand why an error occurred.

.. raw:: html

    <span></span>

* Panic (using ``roc_panic``) when a contract or an invariant is broken. A panic is always preferred over a crash. However, remember that panics are only for bugs in Roc itself. Never panic on invalid or unexpected data from the outside world.

Coding style
============

* The code should be formatted using ``scons fmt``, which invokes ``clang-format``. If it goes awry, you can prevent a file from being formatted by adding it to ``.fmtignore``.

.. raw:: html

    <span></span>

* Header and source files should contain the "Roc authors" copyright and license header. Running ``scons fmt`` will automatically insert them.

.. raw:: html

    <span></span>

* Headers, classes, public members, and free functions should be documented using Doxygen. If Doxygen is installed, it is invoked during build and warns about undocumented items.

.. raw:: html

    <span></span>

* Prefer creating individual .h and .cpp files for every class. Use snake_case for file names and old-style header guards, which are automatically inserted by ``scons fmt``.

.. raw:: html

    <span></span>

* Use upper case SNAKE_CASE for macros, CamelCase for class names, and lower case snake_case for methods, functions, fields, and variables. Add trailing underscore\_ for private methods and fields.

.. raw:: html

    <span></span>

* The code should be formatted according to our 1TBS-like indentation style defined in ``.clang-format`` config:

  * use 4 spaces for indentation;
  * place opening braces on the same line as the control statement;
  * use braces even for single-statement blocks;
  * don't place condition or loop bodies at the same line as the control statement.

.. raw:: html

    <span></span>

* ``#endif`` and ``#else`` statements should have trailing ``// <NAME>`` and ``// !<NAME>`` comments. Namespace closing brace should have trailing ``// namespace <name>`` comment.

.. raw:: html

    <span></span>
