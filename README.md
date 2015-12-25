# Roc: real-time audio streaming over network

Roc is a library and tools for real-time streaming of high-quality audio over unreliable network.

Goals:
* guaranteed latency;
* good quality of service on unreliable networks, such as 802.11 (Wi-Fi);
* portability;
* relying on open, standard protocols.

Roadmap for upcoming 0.1 release
--------------------------------

Work in progress. See also [opened issues](https://github.com/roc-project/roc/issues) and [TODO](TODO.md) for current and future plans.

- [ ] api: public API for client and server
- [x] stack: client/server libraries and tools
- [ ] documentation: API, doxygen, wiki
- [ ] protocols: RTP/RTCP support
- [x] audio formats: uncompressed 16-bit linear PCM
- [x] latency: dynamic resampling in server
- [x] QoS: LDPC forward error correction
- [ ] platforms: Linux (including Raspberry Pi), Mac OS X

Documentation
-------------

* [Wiki](https://github.com/roc-project/roc/wiki)

Building
--------

For build instructions, see [INSTALL](INSTALL.md).

Try it
------

After building, tools are inside `bin/` directory.

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

Contributions are always welcome! Please read [HACKING](HACKING.md) for general hints and look at [TODO](TODO.md) and [opened issues](https://github.com/roc-project/roc/issues) to figure out what's going on.

* If you'd like to report a bug, ask a question, or suggest a feature, feel free to [create an issue](https://help.github.com/articles/creating-an-issue/).

* If you'd like to implement a feature, it's recommended to comment corresponding issue or open a new one to ensure that it fits in Roc design and nobody else is already working on it. Then, just create a [pull request](https://help.github.com/articles/using-pull-requests/).

Licensing
---------

Roc source code is licensed under [MPL-2.0](https://www.mozilla.org/en-US/MPL/2.0/), see [LICENSE](LICENSE).

Issues with external dependencies:
* Roc may be configured to use LDPC-Staircase codec from OpenFEC, which is licensed under [CeCILL](http://openfec.org/patents.html), GPL-like and GPL-compatible license. When Roc is built with OpenFEC support enabled, it must be distributed under lincense compatible with CeCILL.
