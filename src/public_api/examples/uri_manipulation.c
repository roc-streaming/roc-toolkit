/*
 * This example demonstrates how to build endpoint URI and access its individual parts.
 *
 * Building:
 *   cc -o uri_manipulation uri_manipulation.c -lroc
 *
 * Running:
 *   ./uri_manipulation
 *
 * License:
 *   public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <roc/endpoint.h>

#define oops()                                                                           \
    do {                                                                                 \
        fprintf(stderr, "oops: failure on %s:%d\n", __FILE__, __LINE__);                 \
        fprintf(stderr, "exiting!\n");                                                   \
        exit(1);                                                                         \
    } while (0)

static void make_from_string(roc_endpoint* endp) {
    /* Setup endpoint from entire URI string. */
    if (roc_endpoint_set_uri(endp, "rtsp://example.com:123/path?query") != 0) {
        oops();
    }
}

static void make_from_parts(roc_endpoint* endp) {
    /* Setup endpoint from individual URI parts. */
    if (roc_endpoint_set_protocol(endp, ROC_PROTO_RTSP) != 0) {
        oops();
    }
    if (roc_endpoint_set_host(endp, "example.com") != 0) {
        oops();
    }
    if (roc_endpoint_set_port(endp, 123) != 0) {
        oops();
    }
    if (roc_endpoint_set_resource(endp, "/path?query") != 0) {
        oops();
    }
}

static void make_rtsp_uri_without_port_and_resource(roc_endpoint* endp) {
    /* The port is omitted, standard RTSP port will be used.
     * The resource is omitted. */
    if (roc_endpoint_set_uri(endp, "rtsp://example.com") != 0) {
        oops();
    }
}

static void make_rtp_rs8m_ipv4_uri(roc_endpoint* endp) {
    /* RTP header + Reed-Solomon FECFRAME footer, IPv4 host.
     * Port can't be omitted because RTP doesn't define standard port.
     * Resource can't be present because RTP doesn't support it.
     */
    if (roc_endpoint_set_uri(endp, "rtp+rs8m://127.0.0.1:123") != 0) {
        oops();
    }
}

static void make_rtp_ipv6_uri(roc_endpoint* endp) {
    /* RTP header, IPv6 host.
     * Port can't be omitted because RTP doesn't define standard port.
     * Resource can't be present because RTP doesn't support it.
     */
    if (roc_endpoint_set_uri(endp, "rtp://[::1]:123") != 0) {
        oops();
    }
}

static void print_uri(roc_endpoint* endp) {
    /* Determine buffer size. */
    size_t bufsz = 0;
    if (roc_endpoint_get_uri(endp, NULL, &bufsz) != 0 || bufsz == 0) {
        oops();
    }

    /* Allocate buffer. */
    char* buf = malloc(bufsz);
    if (!buf) {
        oops();
    }

    /* Format URI into the buffer. */
    if (roc_endpoint_get_uri(endp, buf, &bufsz) != 0) {
        oops();
    }

    /* Print URI. */
    printf("  uri: %s\n", buf);

    /* Free buffer. */
    free(buf);
}

static void print_parts(roc_endpoint* endp) {
    /* Temporary buffer for strings. */
    size_t bufsz;
    char* buf;

    /* Get and print protocol.
     * A valid URI always has a protocol. */
    roc_protocol proto;
    if (roc_endpoint_get_protocol(endp, &proto) != 0) {
        oops();
    }
    printf("  protocol: %d\n", proto);

    /* Get and print host.
     * A valid URI always has a host. */
    if (roc_endpoint_get_host(endp, NULL, &bufsz) != 0 || bufsz == 0) {
        oops();
    }
    if (!(buf = malloc(bufsz))) {
        oops();
    }
    if (roc_endpoint_get_host(endp, buf, &bufsz) != 0) {
        oops();
    }
    printf("  host: %s\n", buf);

    /* Get and print port, if it is present. */
    int port;
    if (roc_endpoint_get_port(endp, &port) == 0) {
        printf("  port: %d\n", port);
    } else {
        printf("  port: not set\n");
    }

    /* Get and print resource, if it is present. */
    if (roc_endpoint_get_resource(endp, NULL, &bufsz) == 0) {
        if (!(buf = realloc(buf, bufsz))) {
            oops();
        }
        if (roc_endpoint_get_resource(endp, buf, &bufsz) != 0) {
            oops();
        }
        printf("  resource: %s\n", buf);
    } else {
        printf("  resource: not set\n");
    }

    /* Free temporary buffer. */
    free(buf);
}

int main() {
    roc_endpoint* endp = NULL;

    printf("make_from_string:\n");

    if (roc_endpoint_allocate(&endp) != 0) {
        oops();
    }

    make_from_string(endp);
    print_uri(endp);
    print_parts(endp);

    if (roc_endpoint_deallocate(endp) != 0) {
        oops();
    }

    printf("make_from_parts:\n");

    if (roc_endpoint_allocate(&endp) != 0) {
        oops();
    }

    make_from_parts(endp);
    print_uri(endp);
    print_parts(endp);

    if (roc_endpoint_deallocate(endp) != 0) {
        oops();
    }

    printf("make_rtsp_uri_without_port_and_resource:\n");

    if (roc_endpoint_allocate(&endp) != 0) {
        oops();
    }

    make_rtsp_uri_without_port_and_resource(endp);
    print_uri(endp);
    print_parts(endp);

    if (roc_endpoint_deallocate(endp) != 0) {
        oops();
    }

    printf("make_rtp_rs8m_ipv4_uri:\n");

    if (roc_endpoint_allocate(&endp) != 0) {
        oops();
    }

    make_rtp_rs8m_ipv4_uri(endp);
    print_uri(endp);
    print_parts(endp);

    if (roc_endpoint_deallocate(endp) != 0) {
        oops();
    }

    printf("make_rtp_ipv6_uri:\n");

    if (roc_endpoint_allocate(&endp) != 0) {
        oops();
    }

    make_rtp_ipv6_uri(endp);
    print_uri(endp);
    print_parts(endp);

    if (roc_endpoint_deallocate(endp) != 0) {
        oops();
    }

    return 0;
}
