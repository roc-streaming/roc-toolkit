.. roc documentation master file, created by
   sphinx-quickstart on Wed Jun  7 23:03:13 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. image:: ../images/banner.png

Roc: real-time audio streaming over network
*******************************************

Roc is a library and tools for real-time streaming of high-quality audio over unreliable network.

Goals:

* guaranteed latency;
* good quality of service on unreliable networks, such as 802.11 (Wi-Fi);
* portability;
* relying on open, standard protocols.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   overview
   tutorial
   roc_send
   roc_recv
   api
   design
   examples

Try it
------

After building (see file `INSTALL.md <https://github.com/roc-project/roc/wiki/Building-%28native%29>`_), tools are installed into ``bin/<host>/`` directory.

Example usage:

* Start server listening on all interfaces on UDP port 12345:

	``$ rov-recv -vv :12345``

* Send WAV file to server:

	``$ roc-send -vv -i song.wav <server_ip>:12345``

See :ref:`roc_send` and :ref:`roc_recv` for usage details.

