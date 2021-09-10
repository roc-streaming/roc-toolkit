# Hedley

## Documentation

For documentation, see [https://nemequ.github.io/hedley/](https://nemequ.github.io/hedley/).
There is an easy-to-read user guide and full API documentation.

## Brief Description

Hedley is C/C++ a header file designed to smooth over some
platform-specific annoyances.  The idea is to get rid of a bunch of
the #ifdefs in your code and put them in Hedley instead or, if you
haven't bothered with platform-specific functionality in your code, to
make it easier to do so.  This code can be used to improve:

 * Static analysis — better warnings and errors help you catch errors
   before they become a real issue.
 * Optimizations — compiler hints help speed up your code.
 * Manage public APIs
   * Visibility — keeping internal symbols private can make your
     program faster and smaller.
   * Versioning — help consumers avoid functions which are deprecated
     or too new for all the platforms they want to support.
 * C/C++ interoperability — make it easier to use code in both C and
   C++ compilers.
* *… and more!*

You can safely use Hedley in your *public* API.  If someone else
includes a newer version of Hedley later on, the newer Hedley will
just redefine everything, and if someone includes an older version it
will simply be ignored.

It should be safe to use any of Hedley's features; if the platform
doesn't support the feature it will be silently ignored.
