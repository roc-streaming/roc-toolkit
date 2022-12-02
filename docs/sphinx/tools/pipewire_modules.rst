PipeWire modules
****************

PipeWire provides two Roc modules.

===================================================================== ====================================================================
Module                                                                Description
===================================================================== ====================================================================
`roc-source <https://docs.pipewire.org/page_module_roc_source.html>`_ receives audio from network, can be connected to local audio device
`roc-sink <https://docs.pipewire.org/page_module_roc_sink.html>`_     sends audio to network, local audio apps can be connected to it
===================================================================== ====================================================================

The idea is similar to :doc:`Roc PulseAudio modules </tools/pulseaudio_modules>`. Unlike PulseAudio modules, PipeWire modules are not part of Roc, but are provided by PipeWire itself.
