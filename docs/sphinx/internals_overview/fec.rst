Forward erasure correction codes
********************************

.. warning::

   This section is under construction.

Roc is being designed with an idea of sensible latency minimization in sight. That's why roc transmits audio content in UDP packets and why roc incorporates Forward Erasure Correction Codes.

Roc cuts samples flow into blocks and sends several redundant packets along with them so as to recover lost packets. In the example above, roc adds 5 redundant packets to every 10 data packets, so that the roc-recv is able to recover 5 data packets at maximum if they get lost or delayed. On the other hand the data rate is increased in 15/10 times.

Roc doesn't make FEC on its own: it uses `OpenFEC <http://openfec.org/>`_ for that purpose. OpenFEC provides two FEC schemes: Reed-Solomon and LDPC, the former one is more suitable for relatively small latency and small data-rates, therefore it is a default option.

Moreover, the roc's interface of block codec allows attaching another implementation with ease. Feel free to integrate great opensource and free implementation of some effective code.
