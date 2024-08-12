Sponsors
********

Big thanks to people and companies who have sponsored this project!

Becoming a sponsor
==================

If you would like to support the project financially, you can use one of the following services for one-time or recurring donations. Your donations allow to allocate more time for development and buy hardware for testing. Thank you!

* `Open Collective <https://opencollective.com/roc-streaming>`_
* `Liberapay <https://liberapay.com/roc-streaming>`_

If you or your company wants to sponsor development of a particular feature, you're welcome to contact us through the public :ref:`mailing list <mailing_list>` or write directly to one of the :ref:`maintainers <maintainers>`.

Donation policy
===============

This project is developed by volunteers, with huge amount of work being done in their free time. Donations will help us to spend more time on the project and keep it growing.

Received donations are primarily used to sponsor developers time. Donations are accumulated until we collect a meaningful amount suitable for funding development for a few weeks or so.

All work sponsored by donations or customers is documented on this page below.

Corporate sponsors
==================

This section lists companies that have sponsored development and chose to make this information public.

.. image:: https://roc-streaming.org/images/sponsors/boring_tech.png
   :height: 150px
   :target: https://www.boring.tech/

Sponsored features
==================

This section lists features which development was sponsored by companies, individuals, or donations.

.. list-table::
   :widths: 11 8 15 70
   :header-rows: 1

   * - Date
     - Release
     - Source
     - Feature

   * - Aug 2024
     - 0.5.0
     - contract work
     - Adaptive latency support.

   * - Jun 2024
     - 0.5.0
     - contract work
     - Simple PLC & support for registering custom PLC in C API.

   * - May 2024
     -
     - donation by `Sean McNamara <https://github.com/allquixotic>`_
     - Complete first release of macOS virtual audio device (`roc-vad <https://github.com/roc-streaming/roc-vad>`_).

   * - May 2024
     - 0.4.0
     - contract work
     - Support partial reads in pipeline elements.

   * - Apr 2024
     - 0.4.0
     - contract work
     - Support reporting status codes in pipeline elements.

   * - Feb 2024
     - 0.4.0
     - contract work
     - Sender-side latency tuning / clock-drift compensation.

   * - Dec 2023 - Jan 2024
     - 0.4.0
     - contract work
     - Two-way RTCP reports. Sending feedback packets from receiver to sender.

   * - Oct 2023
     - 0.3.0
     - contract work
     - SpeexDEC resampler for cheap clock drift compensation.

   * - Sep 2023
     - 0.3.0
     - contract work
     - Implement end-to-end latency metrics (requires synchronized system clocks).

   * - Aug 2023
     - 0.3.0
     - contract work
     - Encoder / decoder (networkless) API in the C API.

   * - Jul 2023
     - 0.3.0
     - contract work
     - Support operating on lower latencies (remove hard-coded frame sizes, add responsive latency tuning profile).

   * - Jul 2023
     - 0.3.0
     - contract work
     - Support custom encodings (sample rate, channel layout) in the C API.

   * - Jul 2022
     - 0.2.0
     - `Boring Tech <https://www.boring.tech/>`_
     - Various improvements in the C API.

   * - Jun 2022
     - 0.2.0
     - `Boring Tech <https://www.boring.tech/>`_
     - Implement partial RTCP support (reports from sender to receiver).
