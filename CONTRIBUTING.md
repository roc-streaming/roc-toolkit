CONTRIBUTING
============

Don't be afraid not to follow all of the recommendations bellow; someone will assist you when reviewing a pull request. See also our [manifesto](https://github.com/roc-project/roc/wiki/Manifesto).

Conventions
-----------

* **Dependencies**

    We don't use STL and boost. Avoid introducing unnecessary external dependencies. But when it's necessay or obviously useful, try to make it optional. Prefer using cross-platform libraries.

* **Targets**

    Platform-dependent components should be isolated inside `target_` directories. See details [on wiki](https://github.com/roc-project/roc/wiki/Overview#targets). Prefer target directories to #ifdef-s. We use C++98 for the most of the code. Compiler-dependent code should be isolated inside target directories too.

* **Comments**

    Document header files, classes and public members using Doxygen. If doxygen is installed, it is invoked during build and warns about undocumented items.

* **Warnings**

    If possible, ensure you code builds with `--enable-werror` option with major compilers, preferrably recent `clang` and `gcc`. Running `scons tidy` may be also useful. See details in [INSTALL](INSTALL.md).

* **Tests**

    Write unit tests for non-trivial components. Before comitting, run `scons test` to ensure that existing tests are not broken. See details in [INSTALL](INSTALL.md).

* **Error handling**

    Use `roc_panic()` to die gracefully when component's contract is broken. Use `roc_log()` to make debugging your code easier for others.

    We don't use exceptions and avoid returning error codes. Use `roc_panic()` and `roc_log()` and prefrer boolean or null return values.

* **Multi-threading**

    Avoid extensive communications between threads and sharing mutable objects. See details [on wiki](https://github.com/roc-project/roc/wiki/Overview#threads).

* **Code style**

    Source code is periodically formatted using [`format.py`](scripts/format.py) script and `clang-format` with [`.clang-format`](.clang-format) config.

    Follow existing code style or better run `scons fmt` before comitting to avoid unnecessary merge conflicts in future. See details in [INSTALL](INSTALL.md).

* **Copyright**

    Ensure that copyright and MPL-2 header are present in every file. Run `scons fmt` or `scripts/format.py` to insert them automatically.

    You can create `.copyright` file in the root directory of your working copy to provide non-default copyright header for `scripts/format.py`. Also feel free to add your copyright to a file (before MPL-2 header) when introducing non-trivial changes.

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

CI
--

* **Linux**

    Travis is configured to build `master` and `develop` branches and pull requests to them. Builds are running on several Linux distros and compilers using Docker. Docker images are prepared using Docker Hub [automated builds](https://docs.docker.com/docker-hub/builds/).

    Links:
     * [Roc on Docker Hub](https://hub.docker.com/u/rocproject/)
     * [Roc on Travis](https://travis-ci.org/roc-project)
     * [Our dockerfiles on GitHub](https://github.com/roc-project/docker-ci)
     * [Our travis configuration](.travis.yml)
     * [Our travis scripts](scripts/travis)

    You can also run CI builds locally, e.g.:

    ```
    $ docker pull rocproject/ci-fedora
    $ docker run -ti --rm -u "${UID}" -v "${PWD}:${PWD}" -w "${PWD}" rocproject/ci-fedora \
        scons --with-3rdparty=openfec,cpputest variant=debug test
    ```

    Docker images contain uptodate distros with preinstalled dependencies. They are triggered for rebuilt automatically when `Dockerfile` on GitHub changes or mainline docker image is updated. E.g., `ci-fedora` is updated when official `fedora` docker image is updated.

    Configured builds:

    Image                        | Based on        | Platforms   | Compilers
    ---------------------------- | --------------- | ----------- | ---------
    `rocproject/ci-fedora`       | `fedora:latest` | x86_64      | *default*
    `rocproject/ci-debian`       | `debian:stable` | x86_64      | *default*
    `rocproject/ci-ubuntu:14.04` | `ubuntu:14.04`  | x86_64      | gcc-4.4, gcc-4.6, clang-3.4
    `rocproject/ci-ubuntu:16.04` | `ubuntu:16.06`  | x86_64      | gcc-4.8, gcc-4.9, gcc-5.x, clang-3.6, clang-3.7
    `rocproject/ci-ubuntu:nodep` | `ubuntu:16.04`  | x86_64      | *default*
    `rocproject/ci-centos:5`     | `centos:5`      | x86_64      | gcc-4.1
    `rocproject/ci-centos:7`     | `centos:7`      | x86_64      | *default*
    `rocproject/ci-empty`        | `ubuntu:16.04`  | x86_64      | *default*
    `rocproject/cross-raspberry` | `debian:stable` | armv6       | arm-bcm2708hardfp-linux-gnueabi-gcc-4.7
    `rocproject/cross-linaro`    | `debian:stable` | armv7       | arm-linux-gnueabihf-gcc-4.9
