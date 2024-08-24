Reference
*********

.. warning::

   There is no compatibility promise until 1.0.0 is released. Small breaking changes are possible.

.. seealso::

   Alphabetical index is :ref:`available here <genindex>`.

.. contents:: Index:
   :local:
   :depth: 1

roc_context
===========

.. code-block:: c

   #include <roc/context.h>

.. doxygentypedef:: roc_context

.. doxygenfunction:: roc_context_open

.. doxygenfunction:: roc_context_register_encoding

.. doxygenfunction:: roc_context_register_plc

.. doxygenfunction:: roc_context_close

roc_sender
==========

.. code-block:: c

   #include <roc/sender.h>

.. doxygentypedef:: roc_sender

.. doxygenfunction:: roc_sender_open

.. doxygenfunction:: roc_sender_configure

.. doxygenfunction:: roc_sender_connect

.. doxygenfunction:: roc_sender_query

.. doxygenfunction:: roc_sender_unlink

.. doxygenfunction:: roc_sender_write

.. doxygenfunction:: roc_sender_close

roc_receiver
============

.. code-block:: c

   #include <roc/receiver.h>

.. doxygentypedef:: roc_receiver

.. doxygenfunction:: roc_receiver_open

.. doxygenfunction:: roc_receiver_configure

.. doxygenfunction:: roc_receiver_bind

.. doxygenfunction:: roc_receiver_query

.. doxygenfunction:: roc_receiver_unlink

.. doxygenfunction:: roc_receiver_read

.. doxygenfunction:: roc_receiver_close

roc_sender_encoder
==================

.. code-block:: c

   #include <roc/sender_encoder.h>

.. doxygentypedef:: roc_sender_encoder

.. doxygenfunction:: roc_sender_encoder_open

.. doxygenfunction:: roc_sender_encoder_activate

.. doxygenfunction:: roc_sender_encoder_query

.. doxygenfunction:: roc_sender_encoder_push_frame

.. doxygenfunction:: roc_sender_encoder_push_feedback_packet

.. doxygenfunction:: roc_sender_encoder_pop_packet

.. doxygenfunction:: roc_sender_encoder_close

roc_receiver_decoder
====================

.. code-block:: c

   #include <roc/receiver_decoder.h>

.. doxygentypedef:: roc_receiver_decoder

.. doxygenfunction:: roc_receiver_decoder_open

.. doxygenfunction:: roc_receiver_decoder_activate

.. doxygenfunction:: roc_receiver_decoder_query

.. doxygenfunction:: roc_receiver_decoder_push_packet

.. doxygenfunction:: roc_receiver_decoder_pop_feedback_packet

.. doxygenfunction:: roc_receiver_decoder_pop_frame

.. doxygenfunction:: roc_receiver_decoder_close

roc_frame
=========

.. code-block:: c

   #include <roc/frame.h>

.. doxygenstruct:: roc_frame
   :members:

roc_packet
==========

.. code-block:: c

   #include <roc/packet.h>

.. doxygenstruct:: roc_packet
   :members:

roc_endpoint
============

.. code-block:: c

   #include <roc/endpoint.h>

.. doxygentypedef:: roc_endpoint

.. doxygenfunction:: roc_endpoint_allocate

.. doxygenfunction:: roc_endpoint_set_uri

.. doxygenfunction:: roc_endpoint_set_protocol

.. doxygenfunction:: roc_endpoint_set_host

.. doxygenfunction:: roc_endpoint_set_port

.. doxygenfunction:: roc_endpoint_set_resource

.. doxygenfunction:: roc_endpoint_get_uri

.. doxygenfunction:: roc_endpoint_get_protocol

.. doxygenfunction:: roc_endpoint_get_host

.. doxygenfunction:: roc_endpoint_get_port

.. doxygenfunction:: roc_endpoint_get_resource

.. doxygenfunction:: roc_endpoint_deallocate

roc_config
==========

.. code-block:: c

   #include <roc/config.h>

.. doxygentypedef:: roc_slot

.. doxygenvariable:: ROC_SLOT_DEFAULT

.. doxygenenum:: roc_interface

.. doxygenenum:: roc_protocol

.. doxygenenum:: roc_packet_encoding

.. doxygenenum:: roc_fec_encoding

.. doxygenenum:: roc_format

.. doxygenenum:: roc_subformat

.. doxygenenum:: roc_channel_layout

.. doxygenstruct:: roc_media_encoding
   :members:

.. doxygenenum:: roc_clock_source

.. doxygenenum:: roc_latency_tuner_backend

.. doxygenenum:: roc_latency_tuner_profile

.. doxygenenum:: roc_resampler_backend

.. doxygenenum:: roc_resampler_profile

.. doxygenenum:: roc_plc_backend

.. doxygenstruct:: roc_context_config
   :members:

.. doxygenstruct:: roc_sender_config
   :members:

.. doxygenstruct:: roc_receiver_config
   :members:

roc_metrics
===========

.. code-block:: c

   #include <roc/metrics.h>

.. doxygenstruct:: roc_connection_metrics
   :members:

.. doxygenstruct:: roc_sender_metrics
   :members:

.. doxygenstruct:: roc_receiver_metrics
   :members:

roc_plugin
==========

.. code-block:: c

   #include <roc/plugin.h>

.. doxygenenumvalue:: ROC_ENCODING_ID_MIN

.. doxygenenumvalue:: ROC_ENCODING_ID_MAX

.. doxygenenumvalue:: ROC_PLUGIN_ID_MIN

.. doxygenenumvalue:: ROC_PLUGIN_ID_MAX

.. doxygenstruct:: roc_plugin_plc
   :members:

roc_log
=======

.. code-block:: c

   #include <roc/log.h>

.. doxygenenum:: roc_log_level

.. doxygenstruct:: roc_log_message
   :members:

.. doxygentypedef:: roc_log_handler

.. doxygenfunction:: roc_log_set_level

.. doxygenfunction:: roc_log_set_handler

roc_version
===========

.. code-block:: c

   #include <roc/version.h>

.. doxygendefine:: ROC_VERSION_MAJOR

.. doxygendefine:: ROC_VERSION_MINOR

.. doxygendefine:: ROC_VERSION_PATCH

.. doxygendefine:: ROC_VERSION

.. doxygendefine:: ROC_VERSION_CODE

.. doxygenstruct:: roc_version
   :members:

.. doxygenfunction:: roc_version_load
