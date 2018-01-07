![](docs/images/logo.png)

# Roc: real-time audio streaming over network

Roc is a library and tools for real-time streaming of high-quality audio over unreliable network.

Goals:
* guaranteed latency;
* good quality of service on unreliable networks, such as 802.11 (Wi-Fi);
* portability;
* relying on open, standard protocols.

Status
------

Docker images used in Travis are availale on [Docker Hub](https://hub.docker.com/u/rocproject/). See [Continuous integration](https://github.com/roc-project/roc/wiki/Continuous-integration) page for details.

Branch    | Linux
--------- | -----
`master`  | [![](https://travis-ci.org/roc-project/roc.svg?branch=master)](https://travis-ci.org/roc-project/roc)
`develop` | [![](https://travis-ci.org/roc-project/roc.svg?branch=develop)](https://travis-ci.org/roc-project/roc)

Work in progress!
-----------------

There is no public release yet. See [Roadmap](https://roc-project.github.io/docs/development/roadmap.html) and [open issues](https://github.com/roc-project/roc/issues) for current and future plans.

The upcoming 0.1 release will include the following features:

- [x] Simple public API for sender and receiver
- [x] Command line tools for sender and receiver
- [x] Network I/O
- [x] Sound I/O (in tools)
- [x] Audio processing pipeline with dynamic resampling
- [x] RTP support with uncompressed 16-bit linear PCM
- [x] FECFRAME support with Reed-Solomon and LDPC-Staircase [FEC](https://en.wikipedia.org/wiki/Forward_error_correction) codes using [OpenFEC](http://openfec.org/)
- [ ] Documentation
- [x] Linux support, including Raspberry Pi
- [x] Mac OS X support
- [x] Proof of concept Roc-based network transport for PulseAudio

Documentation
-------------

* [Wiki](https://github.com/roc-project/roc/wiki)
* [API](https://github.com/roc-project/roc/tree/develop/src/lib/roc) (*work in progress*)

Building
--------

See examples [on wiki](https://github.com/roc-project/roc/wiki/Building-%28native%29). See also [INSTALL](INSTALL.md) for further details.

Try it
------

After building, tools are instaleld into `bin/<host>/` directory.

Example usage:

* Start receiver listening on all interfaces on UDP ports `10001` and `10002`:

    ```
    $ roc-recv -vv -s :10001 -r :10002
    ```

* Send WAV file to the receiver:

    ```
    $ roc-send -vv -s <receiver_ip>:10001 -r <receiver_ip>:10002 -i file.wav
    ```

See `--help` option for usage details.

Supported platforms
-------------------

* Linux
* Mac OS X (*work in progress*)

There are plans to support other platforms, notably other *nix systems, Android, and maybe some embedded systems like FreeRTOS.

Supported protocols
-------------------

* [RTP](https://tools.ietf.org/html/rfc3550): [A/V Profile](https://tools.ietf.org/html/rfc3551)
* [FECFRAME](https://tools.ietf.org/html/rfc6363): [Reed-Solomon Scheme](https://tools.ietf.org/html/rfc6865) (*work in progress*)
* [FECFRAME](https://tools.ietf.org/html/rfc6363): [LDPC-Staircase Scheme](https://tools.ietf.org/html/rfc6816) (*work in progress*)

There are plans to support RTCP, SAP/SDP, and RTSP in upcoming releases.

Contributing
------------

Contributions are always welcome! Please read [CONTRIBUTING](CONTRIBUTING.md) for general hints and look at [Roadmap](https://roc-project.github.io/docs/development/roadmap.html) and [open issues](https://github.com/roc-project/roc/issues) to figure out what's going on.

* If you'd like to report a bug, ask a question, or suggest a feature, feel free to [create an issue](https://help.github.com/articles/creating-an-issue/).

* If you'd like to submit a patch, [create a pull request](https://help.github.com/articles/using-pull-requests/). For non-trivial changes, it may be reasonable to start with discussing the implementation details in an existing or a new issue.

Licensing
---------

Roc source code is licensed under [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/), see [LICENSE](LICENSE). [Roc logos](docs/images/) by [botanicahouse](https://www.instagram.com/botanicahouse/) are licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

Issues with external dependencies:
* Roc may be configured to use FEC codec from OpenFEC, which is licensed under [CeCILL](http://openfec.org/patents.html), a GPL-like and GPL-compatible license. When Roc is built with OpenFEC support enabled, it must be distributed under a lincense compatible with CeCILL.

Authors
-------

See [AUTHORS](AUTHORS).
