CONTRIBUTING
============

Don't be afraid not to follow all of the recommendations bellow; someone will assist you when reviewing a pull request. See also our [Coding guidelines](https://github.com/roc-project/roc/wiki/Coding-guidelines).

Conventions
-----------

* **Dependencies**

    We don't use STL and boost. Avoid introducing unnecessary external dependencies. But when it's necessary or obviously useful, try to make it optional. Prefer using cross-platform libraries.

* **Targets**

    Platform-dependent components should be isolated inside `target_` directories. See details [on wiki](https://github.com/roc-project/roc/wiki/Overview#targets). Prefer target directories to #ifdef-s. We use C++98 for the most of the code. Compiler-dependent code should be isolated inside target directories too.

* **Comments**

    Document header files, classes and public members using Doxygen. If doxygen is installed, it is invoked during build and warns about undocumented items.

* **Warnings**

    If possible, ensure that your code builds with `--enable-werror` option with major compilers, preferably recent `clang` and `gcc`. Running `scons tidy` may be also useful. See details in [INSTALL](INSTALL.md).

* **Tests**

    Write unit tests for non-trivial components. Before committing, run `scons test` to ensure that existing tests are not broken. See details in [INSTALL](INSTALL.md).

* **Error handling**

    Use `roc_panic()` to die gracefully when component's contract is broken. Use `roc_log()` to make debugging your code easier for others.

    We don't use exceptions and avoid returning error codes. Use `roc_panic()` and `roc_log()` and prefer boolean or null return values.

* **Multi-threading**

    Avoid extensive communications between threads and sharing mutable objects. Prefer thread-safe queues and transfer object ownership to them. See details [on wiki](https://github.com/roc-project/roc/wiki/Overview#threads).

* **Code style**

    Source code is periodically formatted using [`format.py`](scripts/format.py) script and `clang-format` with [`.clang-format`](.clang-format) config.

    Follow existing code style and if possible run `scons fmt` before committing to avoid unnecessary merge conflicts in future. See details in [INSTALL](INSTALL.md).

* **Copyright**

    Ensure that copyright and MPL-2 header are present in every file. Run `scons fmt` or `scripts/format.py` to insert them automatically.

    You can create `.copyright` file in the root directory of your working copy to provide non-default copyright header for `scripts/format.py`. Feel free to add your copyright when introducing non-trivial changes.

* **Commits**

    Group independent changes, like formatting, refactoring, bugfixes, and adding new features into separate commits. Rebase your branch on top of target branch before creating a pull request.

Branching
---------

* **Branches**

    There are two main branches:
    * `master` is a [protected branch](https://help.github.com/articles/about-protected-branches/) for code tested on all supported platforms. Public releases are tagged commits in `master` branch. History is always linear (no non-fast-forward merges) in this branch.
    * `develop` is an unstable development branch. Changes from `develop` are merged into `master` when they are ready. This branch is sometimes rebased and force-pushed.

    Additionally, feature branches may be created for long-standing development. They are like `develop` branch, but may be incomplete, broken, etc. for a longer time.

    We use rebase workflow with linear history. Please use `git pull --rebase` instead of `git pull`.

* **Pull requests**

    Create pull requests for `develop` branch, not for `master` branch, which is the default.

    If your changes are far from complete but it's make sense to share them and keep in the main repository, create pull request for a feature branch instead. If there is no feature branch fitting your needs, feel free to create an issue and ask to create a new branch.

* **Continuous integration**

    CI automatically builds Roc for several platforms. See details [on wiki](https://github.com/roc-project/roc/wiki/Continuous-integration).
