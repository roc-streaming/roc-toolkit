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

Branch    | Linux
--------- | -----
`master`  | [![](https://travis-ci.org/roc-project/roc.svg?branch=master)](https://travis-ci.org/roc-project/roc)
`develop` | [![](https://travis-ci.org/roc-project/roc.svg?branch=develop)](https://travis-ci.org/roc-project/roc)

Work in progress!
-----------------

There is no public release yet. The upcoming 0.1 release will include the following features:

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

See also [Roadmap](https://roc-project.github.io/docs/development/roadmap.html) page.

Documentation
-------------

* [Roc documentation](https://roc-project.github.io/docs/)
* [Roc internal modules](https://roc-project.github.io/modules/)

Building
--------

See [Building](https://roc-project.github.io/docs/building.html) page. In particular, [Quick start](https://roc-project.github.io/docs/building/quick_start.html) page provides examples for popular distros.

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
* Mac OS X

There are plans to support other platforms, notably other *nix systems, Android, and maybe some embedded systems like FreeRTOS.

Supported protocols
-------------------

* [RTP](https://tools.ietf.org/html/rfc3550): [A/V Profile](https://tools.ietf.org/html/rfc3551)
* [FECFRAME](https://tools.ietf.org/html/rfc6363): [Reed-Solomon Scheme](https://tools.ietf.org/html/rfc6865)
* [FECFRAME](https://tools.ietf.org/html/rfc6363): [LDPC-Staircase Scheme](https://tools.ietf.org/html/rfc6816)

There are plans to support RTCP, SAP/SDP, and RTSP in upcoming releases.

Contributing
------------

Contributions are always welcome!

Feel free to open issues for bug reports, feature requests, and questions.

Pull requests are welcome as well. For large features, it may be reasonable to open an issue and discuss the implementation first. [Development](https://roc-project.github.io/docs/development.html) page provides some details about our plans, conventions, workflow, and tools.

Licensing
---------

Roc source code is licensed under [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/). Roc PulseAudio modules are licensed under [LGPL-2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html). Roc logos provided by [botanicahouse](https://www.instagram.com/botanicahouse/) are licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

Roc by default is configured to use FEC codec from OpenFEC, which is licensed under [CeCILL](http://openfec.org/patents.html), a GPL-like and GPL-compatible license. When Roc is built with OpenFEC support enabled, it must be distributed under a lincense compatible with CeCILL.

Authors
-------

Maintainers and contributors are listed in [AUTHORS](AUTHORS).
