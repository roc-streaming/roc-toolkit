Coding guidelines
*****************

.. contents:: Table of contents:
   :local:
   :depth: 1

Language and libraries
======================

The usage of C++ in this project is quite specific. The codebase is primarily written in what we refer to as "C with classes", a simplified subset of C++ that intentionally omits several of its features:

* No C++ features beyond C++98
* No exceptions
* No STL (the algorithms-and-containers part of the standard library)
* No general-purpose utility libraries like Boost
* Lightweight "core" library (roc_core) instead of STL
* Templates are mostly avoided (except utility classes in core)
* Inheritance is primarily used only for "interface inheritance"
* Overloading, operators, default arguments are avoided

The roc_core library makes several essential design choices that differ significantly from STL:

* it is small and lightweight
* heavy operations, like deep copying or allocations, are never implicit
* most operations include safety checks and will trigger a panic with stacktrace on incorrect usage
* most containers are based on intrusive data structures
* fine-grained memory management based on arenas and pools
* building blocks for lock-free programming

These design choices render Roc codebase pretty different from both plain C and modern C++ code.

This approach may not be suitable for every project, but it appears to have been effective in the case of Roc due to its low-level, real-time nature, and at the same time the considerable size of its codebase.

Portability
===========

* The code should run on a variety of operating systems, compilers, and hardware architectures, including rather old compilers and distributions. See :doc:`supported platforms </portability/supported_platforms>` page.

.. raw:: html

    <span></span>

* The code specific to platform, compiler, or optional features and dependencies, should be isolated inside corresponding :ref:`target directories <targets>`. All other code should be portable across all supported configurations.

Best practices
==============

* The code should compile without warnings. Use ``--enable-werror`` :doc:`option </building/scons_options>` to turn warnings into errors.

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

* Use ``const`` when it's useful.

.. raw:: html

    <span></span>

* Use anonymous namespaces instead of static globals, functions, and constants.

.. raw:: html

    <span></span>

* Use enums instead of defines, when possible.

.. raw:: html

    <span></span>

* Use arenas and pools for memory management.

.. raw:: html

    <span></span>

* Carefully log (using ``roc_log``) all important events and information needed to understand why an error occurred.

.. raw:: html

    <span></span>

* Panic (using ``roc_panic``) when a contract or an invariant is broken. A panic is always preferred over a crash or undefined behavior. However, remember that panics are only for bugs in Roc itself. Never panic on invalid or unexpected data from the outside world.

Coding style
============

* The code should be formatted using ``scons fmt``, which invokes ``clang-format``. If it goes awry, you can prevent a file from being formatted by adding it to ``.fmtignore``.

.. raw:: html

    <span></span>

* Header and source files should contain the "Roc Streaming authors" copyright and license header. Running ``scons fmt`` will automatically insert them.

.. raw:: html

    <span></span>

* Headers, classes, public members, and free functions should be documented using Doxygen. Use ``--enable-doxygen`` :doc:`option </building/scons_options>` to enable warnings about undocumented elements.

.. raw:: html

    <span></span>

* Prefer creating individual .h and .cpp files for every class. Use snake_case for file names and old-style header guards, which are automatically inserted by ``scons fmt``.

.. raw:: html

    <span></span>

* Use upper case SNAKE_CASE for macros, CamelCase for class names, and lower case snake_case for methods, functions, fields, and variables. Add trailing underscore\_ for private methods and fields.

.. raw:: html

    <span></span>

* Members in class should have the following order:

  * public members:
     * types and constants
     * methods

  * protected members:
     * types and constants
     * methods

  * private members:
     * types and constants
     * methods
     * fields

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
