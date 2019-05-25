![](docs/images/logo.png)

# Roc: real-time streaming over the network

Roc is a toolkit for real-time media streaming over the network.

You can read about the project on the [overview](https://roc-project.github.io/roc/docs/about_project/overview.html) and [features](https://roc-project.github.io/roc/docs/about_project/features.html) pages.

The toolkit consists of:

* a C library;
* a set of command-line tools;
* a set of PulseAudio modules.

Key features:

* real-time streaming with guaranteed latency;
* converting between the sender and receiver clock domains;
* restoring lost packets using Forward Erasure Correction codes;
* CD-quality audio;
* multiple profiles for different CPU and latency requirements;
* portability;
* relying on open, standard protocols.

Building
--------

See documentation [here](https://roc-project.github.io/roc/docs/building.html).

In particular, see [project dependencies](https://roc-project.github.io/roc/docs/building/dependencies.html) and [user instructions](https://roc-project.github.io/roc/docs/building/user_cookbook.html).

Running
-------

Read the documentation and examples for [command-line tools](https://roc-project.github.io/roc/docs/running/command_line_tools.html) and [PulseAudio modules](https://roc-project.github.io/roc/docs/running/pulseaudio_modules.html).

See also our [manual pages](https://roc-project.github.io/roc/docs/manuals.html).

API
---

API reference and examples can be found [here](https://roc-project.github.io/roc/docs/api.html).

Platforms
---------

See [documentation](https://roc-project.github.io/roc/docs/portability.html) on the supported platforms.

If you want to run Roc on an embedded system or a single-board computer, take a look at the [tested boards](https://roc-project.github.io/roc/docs/portability/tested_boards.html) and [cross compiling](https://roc-project.github.io/roc/docs/portability/cross_compiling.html) pages.

Documentation
-------------

* [Documentation](https://roc-project.github.io/roc/docs/)
* [Doxygen](https://roc-project.github.io/roc/doxygen/)

Releases
--------

See [releases](https://github.com/roc-project/roc/releases) page for the list of released versions and [changelog](https://roc-project.github.io/roc/docs/development/changelog.html) page for the detailed list of changes.

Versions are assigned accroding to [semantic versioning](https://semver.org/). There is no compatibility promise for the public API until 1.0.0 is released. Small breaking changes are possible.

Releases are tagged from the master branch, which is maintained to be stable. Actual development happens in the develop branch which is merged into master from time to time.

Plans
-----

See the [roadmap](https://roc-project.github.io/roc/docs/development/roadmap.html) and the [project board](https://github.com/roc-project/roc/projects/2).

Continuous integration
----------------------

See details [here](https://roc-project.github.io/roc/docs/development/continuous_integration.html).

Branch    | Status
--------- | ------
`master`  | [![](https://travis-ci.org/roc-project/roc.svg?branch=master)](https://travis-ci.org/roc-project/roc)
`develop` | [![](https://travis-ci.org/roc-project/roc.svg?branch=develop)](https://travis-ci.org/roc-project/roc)

Community
---------

We have an announcements and discussion mailing list for users and developers: roc@freelists.org.

You can browse the list archive and join the list by visiting [this page](https://www.freelists.org/list/roc). You can also join the list by sending an email to roc-request@freelists.org with "subscribe" in the "Subject" field. You need to join to be able to post.

Contributing
------------

Contributions are always welcome!

We use GitHub for issue tracking and code review. Feel free to file bug reports or feature requests, and send pull requests for review. The project internals are documented [here](https://roc-project.github.io/roc/docs/internals.html) and [here](https://roc-project.github.io/roc/doxygen/). The developer's information can be found [here](https://roc-project.github.io/roc/docs/development.html).

Licensing
---------

Roc source code is licensed under [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/). Roc PulseAudio modules are licensed under [LGPL-2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html).

If Roc is built with OpenFEC support enabled, it must be distributed under a lincense compatible with [CeCILL](http://openfec.org/patents.html), a GPL-like and GPL-compatible license used for OpenFEC.

Authors
-------

Maintainers and contributors are listed in [AUTHORS](AUTHORS).
