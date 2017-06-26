![](doc/images/logo.png)

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

Roadmap for upcoming 0.1 release
--------------------------------

Work in progress. See also [open issues](https://github.com/roc-project/roc/issues) and [TODO](TODO.md) for current and future plans.

- [ ] api: public API for client and server
- [x] stack: client/server libraries and tools
- [ ] documentation: API, doxygen, wiki
- [ ] protocols: RTP support
- [x] audio formats: uncompressed 16-bit linear PCM
- [x] latency: dynamic resampling in server
- [x] QoS: LDPC forward error correction
- [ ] platforms: Linux (including Raspberry Pi), Mac OS X

Documentation
-------------

* [Wiki](https://github.com/roc-project/roc/wiki)

Building
--------

See examples [on wiki](https://github.com/roc-project/roc/wiki/Building-%28native%29). See also [INSTALL](INSTALL.md) for further details.

Try it
------

After building, tools are instaleld into `bin/<host>/` directory.

Example usage:

* Start server listening on all interfaces on UDP port `12345`:

    ```
    $ rov-recv -vv :12345
    ```

* Send WAV file to server:

    ```
    $ roc-send -vv -i song.wav <server_ip>:12345
    ```

See `--help` option for usage details.

Supported platforms
-------------------

Currently only Linux is supported. There are plans to add support for other *nix, Mac OS X, Windows, and embedded platforms like FreeRTOS.

Supported protocols
-------------------

*work in progress*

Contributing
------------

Contributions are always welcome! Please read [CONTRIBUTING](CONTRIBUTING.md) for general hints and look at [TODO](TODO.md) and [open issues](https://github.com/roc-project/roc/issues) to figure out what's going on.

* If you'd like to report a bug, ask a question, or suggest a feature, feel free to [create an issue](https://help.github.com/articles/creating-an-issue/).

* If you'd like to submit a patch, it's recommended to comment corresponding issue or create a new one to ensure that your work fits in Roc design and nobody else is already implementing it. Then, just create a [pull request](https://help.github.com/articles/using-pull-requests/).

Licensing
---------

Roc source code is licensed under [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/), see [LICENSE](LICENSE). [Roc logos](doc/images/) by [botanicahouse](https://www.instagram.com/botanicahouse/) are licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

Issues with external dependencies:
* Roc may be configured to use LDPC-Staircase codec from OpenFEC, which is licensed under [CeCILL](http://openfec.org/patents.html), GPL-like and GPL-compatible license. When Roc is built with OpenFEC support enabled, it must be distributed under lincense compatible with CeCILL.

Authors
-------

See [AUTHORS](AUTHORS).
