Contribution guidelines
***********************

.. contents:: Table of contents:
   :local:
   :depth: 1

Community
=========

Welcome to join our Matrix chats and mailing list!

Please refer :doc:`this page </about_project/contacts>` for more details.

How you can help
================

* **Contributing code**

  We are always happy to meet new people in the project. Please refer to the section below for details on the onboarding process.

* **Testing**

  We also appreciate help in testing new releases. We highly depend on feedback from users to identify bugs and receive suggestions for new features.

* **Writing tutorials**

  If you've built Roc for a niche OS or hardware, or tweaked settings to make it work better for your needs, we'd love if you share your experience with the community. If you wrote a guide, you can send it to us to be added to the :doc:`publications page </about_project/publications>`.

Becoming a contributor
======================

First of all, thank you for your interest! We welcome and appreciate all help. Many important features were submitted by :doc:`numerous contributors </about_project/authors>`, and we are really grateful for that.

The guide bellow will help you to prepare your first patch.

* **Step 1: Checkout and build project**

  Build instructions can be found here:

  * :doc:`/building/user_cookbook`
  * :doc:`/building/developer_cookbook`

* **Step 2: Get introduced to project internals**

  Take a look at documentation of :doc:`project internals </internals>`.

  There is also Doxygen-generated documentation for `internal modules <https://roc-streaming.org/toolkit/doxygen/>`_.

* **Step 3: Learn coding guidelines**

  Please refer to this page: :doc:`/development/coding_guidelines`.

* **Step 4: Choose a task**

  Tasks needing assistance are marked with `"help wanted" <https://github.com/roc-streaming/roc-toolkit/labels/help%20wanted>`_ label. These tasks usually come with detailed explanations and are suitable for newcomers.

  Some of these tasks are also labeled as `"easy hacks" <https://github.com/roc-streaming/roc-toolkit/labels/easy%20hacks>`_. It indicates that the solution is expected to be pretty straightforward, making them a good entry point to the project.

  Extra labels make it easier to find various kinds of problems, like "tests," "refactoring," "algorithms," "networking," and so on.

  When you pick a task, please leave a comment to let others know so that we don't end up with multiple people doing the same thing. If a task hasn't been assigned, it usually means no one is currently working on it.

* **Step 5: Create pull request!**

  Please remember that pull requests should be always rebased on ``develop`` branch and should be targeted to it, as :doc:`described here </development/version_control>`.

  Before submitting PR, don't forget to run code formatting, as described in coding guidelines.

  Please ensure that all CI checks pass on your PR and fix them if needed.
