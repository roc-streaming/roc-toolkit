Bindings
********

Languages
---------

In addition to the :doc:`C library </api/reference>`, Roc provides bindings to several other languages.

========================================================== ===========================================
Repo                                                       Description
========================================================== ===========================================
`roc-go <https://github.com/roc-streaming/roc-go>`_        bindings for Go (golang)
`roc-java <https://github.com/roc-streaming/roc-java>`_    bindings for Java, Android support included
========================================================== ===========================================

Versioning
----------

Both C library and bindings use `semantic versioning <https://semver.org/>`_.

Bindings are compatible with the C library if their major version is the same, and minor version is either the same or higher.

For example, version 1.2.3 of the bindings would be compatible with 1.2.x and 1.3.x of the C library, but not with 1.1.x (minor version is lower) or 2.x.x (major version is different).
