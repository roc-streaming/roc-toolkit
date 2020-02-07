Reference
*********

.. warning::

   There is no compatibility promise until 1.0.0 is released. Small breaking changes are possible.

.. contents:: Index:
   :local:
   :depth: 1

roc_context
===========

.. code-block:: c

   #include <roc/context.h>

.. doxygentypedef:: roc_context

.. doxygenfunction:: roc_context_open

.. doxygenfunction:: roc_context_close

roc_sender
==========

.. code-block:: c

   #include <roc/sender.h>

.. doxygentypedef:: roc_sender

.. doxygenfunction:: roc_sender_open

.. doxygenfunction:: roc_sender_set_outgoing_address

.. doxygenfunction:: roc_sender_set_broadcast_enabled

.. doxygenfunction:: roc_sender_set_squashing_enabled

.. doxygenfunction:: roc_sender_connect

.. doxygenfunction:: roc_sender_write

.. doxygenfunction:: roc_sender_close

roc_receiver
============

.. code-block:: c

   #include <roc/receiver.h>

.. doxygentypedef:: roc_receiver

.. doxygenfunction:: roc_receiver_open

.. doxygenfunction:: roc_receiver_set_multicast_group

.. doxygenfunction:: roc_receiver_bind

.. doxygenfunction:: roc_receiver_read

.. doxygenfunction:: roc_receiver_close

roc_frame
=========

.. code-block:: c

   #include <roc/frame.h>

.. doxygentypedef:: roc_frame
   :outline:

.. doxygenstruct:: roc_frame
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

.. doxygentypedef:: roc_interface
   :outline:

.. doxygenenum:: roc_interface

.. doxygentypedef:: roc_protocol
   :outline:

.. doxygenenum:: roc_protocol

.. doxygentypedef:: roc_fec_code
   :outline:

.. doxygenenum:: roc_fec_code

.. doxygentypedef:: roc_packet_encoding
   :outline:

.. doxygenenum:: roc_packet_encoding

.. doxygentypedef:: roc_frame_encoding
   :outline:

.. doxygenenum:: roc_frame_encoding

.. doxygentypedef:: roc_channel_set
   :outline:

.. doxygenenum:: roc_channel_set

.. doxygentypedef:: roc_resampler_profile
   :outline:

.. doxygenenum:: roc_resampler_profile

.. doxygentypedef:: roc_context_config
   :outline:

.. doxygenstruct:: roc_context_config
   :members:

.. doxygentypedef:: roc_sender_config
   :outline:

.. doxygenstruct:: roc_sender_config
   :members:

.. doxygentypedef:: roc_receiver_config
   :outline:

.. doxygenstruct:: roc_receiver_config
   :members:

roc_log
=======

.. code-block:: c

   #include <roc/log.h>

.. doxygentypedef:: roc_log_level
   :outline:

.. doxygenenum:: roc_log_level

.. doxygentypedef:: roc_log_handler

.. doxygenfunction:: roc_log_set_level

.. doxygenfunction:: roc_log_set_handler
