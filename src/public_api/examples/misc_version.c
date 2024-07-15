/*
 * This example demonstrates how to check library version at compile-time and run-time.
 *
 * Building:
 *   cc -o misc_version misc_version.c -lroc
 *
 * Running:
 *   ./misc_version
 *
 * License:
 *   public domain
 */

#include <stdio.h>

#include <roc/version.h>

int main() {
    /* Inspect compile-time library version, defined in header file. */
    printf("compile-time version: %u.%u.%u, version code: %u\n", ROC_VERSION_MAJOR,
           ROC_VERSION_MINOR, ROC_VERSION_PATCH, ROC_VERSION);

#if ROC_VERSION >= ROC_VERSION_CODE(0, 3, 0)
    printf("compile-time version is >= 0.3.0\n");
#else
    printf("compile-time version is < 0.3.0\n");
#endif

    /* Inspect run-time library version, returned by a function. */
    roc_version version;
    roc_version_load(&version);

    printf("run-time version: %u.%u.%u, version code: %u\n", version.major, version.minor,
           version.patch, version.code);

    if (version.code >= ROC_VERSION_CODE(0, 3, 0)) {
        printf("run-time version is >= 0.3.0\n");
    } else {
        printf("run-time version is < 0.3.0\n");
    }

    /* Compare compile-time and run-time version.
     * They may differ when using a shared library. */
    if (version.code == ROC_VERSION) {
        printf("compile-time and run-time versions match\n");
    } else {
        printf("compile-time and run-time versions differ\n");
    }

    return 0;
}
