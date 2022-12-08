Threads
*******

.. contents:: Table of contents:
   :local:
   :depth: 1

Execution threads
=================

Both Roc sender and receiver use threads of up to three types:

* network I/O thread running event loop, capable of sending and receiving packets via multiple sockets;

* processing pipeline thread running sender or receiver pipeline, converting packet stream to audio stream or vice versa;

* sound I/O thread running audio device reader or writer.

Roc command-line tools create these three threads. The network I/O and the processing pipeline threads are managed by Roc itself (though the I/O part is done by libuv), and the sound I/O thread is managed by SoX library. The presence of the sound I/O thread is optional and depends on the selected SoX backend.

Roc library, in contrast, creates only the network I/O thread. The processing pipeline is running in the caller context when the user reads or writes audio stream. The sound I/O is not performed at all and users should implement it by themselves using other available libraries dedicated for that purpose.

It is also worth to mention that Roc library allows both creating individual network I/O threads for every sender or receiver, and sharing a single network I/O thread between multiple senders or receivers.

Threads typically don't share much state and mostly communicate via queues. The network I/O and the processing pipeline threads communicate via packet queues for incoming and outgoing packets. The sound I/O and the processing pipeline threads communicate via a stream buffer which implementation is specific for the selected SoX backend.

The following diagram gives a conceptual view of both sender and receiver structure:

.. image:: ../_images/threads.png
    :align: center
    :alt: Threads

The difference is that on the receiver data is transferring from the network to the sound card and on the sender it is transferring from the sound card to the network.
