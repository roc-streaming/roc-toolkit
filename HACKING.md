HACKING
=======

Objectives
----------

These are objectives, not strict rules. World is complex, people are tired, and objectives are not always met. But we try to keep them in mind.

* **Be lightweight**

    Prefer simple solutions to cute ones. Write code that introduce minimum overhead for solving its task. Write code that can easily be optimized, when it becomes necessary.

    * Use straightforward algorithms and data structures if they play well. Do you need a hash table there, or is linear search enought? In most cases it is, because number of objects is low.

    * Use ascetic memory allocation schemes. Don't use heap if you don't have to. Use memory pools. Use fixed-size arrays instead of dynamic ones, if they play well. Use intrusive lists instead of heap-based ones, if they play well.

* **Be portable**

    Write platform-independent code. If you can't, extract minimum platform-dependent part and isolate it.

* **Keep code base clean**

    Write neat code.

    * Ensure you code compiles without warnings and is properly formatted. Run static analyzer from time to time, and fix its warnings too (well, if it's not going mad).

    * Don't fall into #ifdef hell. Instead, isolate platform-dependent code in separate classes and move them to platform subdirectories.

    * Ensure your methods are short and has self-explaining names.

    * As usual, don't optimize before profiling. Instead, try to structure your code so that optimizations will be obvious and isolated.

* **Keep interfaces simple**

    Write modular code with simple and well-defined interfaces.

    * Ensure your interfaces are minimal, clean, and documented.

    * Prefer simple interfaces and smart implementations, rather than bloated interfaces and lazy implementations.

    * Do your best to keep interface contract straightforward.

* **Write robust code**

    * Check for corner cases. Check for null pointers and overflows. Add runtime checks for everything that can break your code. If contract is broken, die gracefully with readable error message. Otherwise, ensure that code will continue working correctly.

    * Write unit tests for non-trivial components. Write integration tests. Make your components friendly for testing.

Conventions
-----------

Please try to follow conventions bellow when contributing.

* **Dependencies**

    We don't use STL, boost, or so, to keep code transparent, manageable and lightweight. Avoid introducing unnecessary external dependencies. But when it's necessay or obviously useful, try to make it optional.

* **Targets**

    Platform-depended components should be isolated inside `target_` directories. See details [on wiki](https://github.com/roc-project/roc/wiki/Overview#targets). Prefer target directories to #ifdef-s. We use C++98 for the most of the code. Compiler-dependent code should be isolated inside target directories too.

* **Comments**

    Document headers, classes and public members using Doxygen. If doxygen is installed, it is invoked during build and warns about undocumented items.

* **Warnings**

    Ensure you code builds with `--enable-werror` option with major compilers, preferrably `clang` and `gcc`. Running `scons tidy` may be useful. See details in [INSTALL](INSTALL.md).

* **Tests**

    Write unit tests for non-trivial components. Before comitting, run `scons test` to ensure that existing tests are not broken. See details in [INSTALL](INSTALL.md).

* **Debugging**

    Use `roc_panic()` to die gracefully when component's contract is broken. Use `roc_log()` to make debugging your code by other people easier.

* **Code style**

    Source code is periodically formatted using [`format.py`](scripts/format.py) script and `clang-format` with [`.clang-format`](.clang-format) config.

    Follow existing code style or better run `scons fmt` before comitting, to avoid unnecessary merge conflicts in future. See details in [INSTALL](INSTALL.md).

* **Copyright**

    Ensure that copyright and MPL-2 header are present in every file. Run `scons fmt` or `scripts/format.py` to insert them automatically.

    You can create `.copyright` file in the root directory of your working copy to provide non-default copyright header. Also feel free to add your copyright to a file when introducing non-trivial changes.

* **Commits**

    Group independent changes, like formatting, refactoring, bugfixes, and adding new features into separate commits. Rebase your branch on top of target branch before creating a pull request.

Branching
---------

* **Branches**

    There are two main branches:
    * `master` is [protected](https://help.github.com/articles/about-protected-branches/) branch for code tested on all supported platforms. Public releases are tagged commits in `master` branch. History in always kept linear in this branch.
    * `develop` is unstable development branch. Changes from `develop` are merged into `master` when they are ready. This branch is periodically rebased, so be carefull when pulling!

    Additionally, feature branches may be created for long-standing development. They are like `develop` branch, but may be incomlete, broken, etc. for a longer time.

* **Pull requests**

    Create pull requests for `develop` branch, not for `master` branch, which is default.

    If your changes are far from complete but it's make sense to share them and keep in the main repository, create pull request for a feature branch instead. If there is no feature branch fitting your needs, feel free to create an issue and ask to create a new branch.
