Contribution guidelines
***********************

.. contents:: Table of contents:
   :local:
   :depth: 1

How you can help
================

* **Contributing code**

  We are always happy to meet new people in the project. For details on the onboarding process, see below.

* **Testing**

  We appreciate help in testing on various OSes and hardware, and highly depend on feedback from users. If you wish to test, you can look for tutorials on :doc:`publications page </about_project/publications>`.

* **Writing**

  If you've built Roc for a niche OS or hardware, or tweaked settings to make it work better for your needs, or used it in your project, we'd love if you share your experience with the community. If you wrote a post, you can send it to us to be added to the :doc:`publications page </about_project/publications>`.

Becoming a contributor
======================

First of all, thank you for your interest! We welcome and appreciate all help. Many important features were submitted by :doc:`numerous contributors </about_project/authors>`, and we are really grateful for that.

In case of any questions, welcome to join our :doc:`chat and mailing list </about_project/contacts>`.

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

  See :doc:`/development/workflow` page for rules of pull request creation and its life-cycle.

  Please remember that pull requests should be always based on ``develop`` branch!

  Before submitting PR, don't forget to run code formatting, as described in coding guidelines. After submitting, ensure that all CI checks pass on your PR and fix them if needed.
