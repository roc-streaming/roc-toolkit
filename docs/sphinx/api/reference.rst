Reference
*********

.. warning::

   This section is under construction.

.. contents:: Index:
   :local:
   :depth: 1

roc_context
===========

.. code-block:: c

   #include <roc/context.h>

.. doxygentypedef:: roc_context

.. doxygenfunction:: roc_context_open

.. doxygenfunction:: roc_context_start

.. doxygenfunction:: roc_context_stop

.. doxygenfunction:: roc_context_close

roc_sender
==========

.. code-block:: c

   #include <roc/sender.h>

.. doxygentypedef:: roc_sender

.. doxygenfunction:: roc_sender_open

.. doxygenfunction:: roc_sender_bind

.. doxygenfunction:: roc_sender_connect

.. doxygenfunction:: roc_sender_write

.. doxygenfunction:: roc_sender_close

roc_frame
=========

.. code-block:: c

   #include <roc/frame.h>

.. doxygentypedef:: roc_frame
   :outline:

.. doxygenstruct:: roc_frame
   :members:

roc_address
===========

.. code-block:: c

   #include <roc/address.h>

.. doxygentypedef:: roc_family
   :outline:

.. doxygenenum:: roc_family

.. doxygentypedef:: roc_address
   :outline:

.. doxygenstruct:: roc_address
   :members:

.. doxygenfunction:: roc_address_init

.. doxygenfunction:: roc_address_family

.. doxygenfunction:: roc_address_ip

.. doxygenfunction:: roc_address_port

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
