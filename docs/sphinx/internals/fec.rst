Forward Erasure Correction codes
********************************

Roc is being designed with an idea of sensible latency minimization in sight. That's why Roc transmits audio content in UDP packets and why Roc incorporates Forward Erasure Correction codes.

Roc cuts samples flow into blocks and sends several redundant packets along with them so as to recover lost packets. For example, if the sender adds 5 redundant packets to every 10 data packets, the receiver is able to recover 5 data packets at maximum if they get lost or delayed. On the other hand, the data rate is increased in 15/10 times.

Roc implements the FECFRAME specification with several FEC schemes. The network part is implemented in the Roc itself, while the codec part is implemented in `OpenFEC <http://openfec.org/>`_ library. Currently, it's recommended to use `our fork <https://github.com/roc-project/openfec>`_ instead of the upstream version since it provides several bug fixes and minor improvements that are not available in the upstream yet.

Roc provides two FEC schemes:

* Reed-Solomon, suitable for small block sizes and latency, and small data rates
* LDPC-Staircase, suitable for large block sizes and latency, but operating at higher rates

The Roc's interface of a block codec allows attaching another implementation with ease. Feel free to integrate great opensource and free implementation of some effective code.
