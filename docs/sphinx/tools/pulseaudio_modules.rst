PulseAudio modules
******************

`roc-pulse <https://github.com/roc-streaming/roc-pulse>`_ repo implements Roc PulseAudio modules.

========================= ====================================================================
Module                    Description
========================= ====================================================================
``module-roc-sink-input`` receives audio from network, can be connected to local audio device
``module-roc-sink``       sends audio to network, local audio apps can be connected to it
========================= ====================================================================

Highlights:

* Compared to command-line tools, these modules better integrate with PulseAudio. You can connect roc receiver or sender to local device or app via usual PulseAudio GUI and CLI tools, for example ``pavucontrol``.

* Compared to builtin PulseAudio network transports ("native" and "rtp"), Roc modules provide better service quality (less losses and glitches) over unreliable networks like Wi-Fi.

* Compared to builtin PulseAudio network transports, Roc modules allow you to interconnect different systems. They are fully interoperable with other Roc receivers and senders based on libroc, e.g. Roc :doc:`command-line tools </tools/command_line_tools>` or :doc:`Roc Droid </tools/android_app>` app.

.. note::

   In previous releases, PulseAudio modules were part of ``roc-toolkit`` repo. Now they live in separate repo and have independent build system, documentation, and versioning.
