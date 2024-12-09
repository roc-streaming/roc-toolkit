#include <CppUTest/TestHarness.h>
#include "roc_core/stddefs.h"
#include "roc/endpoint.h"
#include "roc/sender.h"
#include "roc/receiver.h"

namespace roc {
namespace api {

TEST(SenderReceiverIntegration, ProxyWithControlEndpoint) {
    roc_context* context;
    roc_proxy* proxy;

    roc_context_config context_config;
    memset(&context_config, 0, sizeof(context_config));
    CHECK(roc_context_open(&context_config, &context) == 0);
    CHECK(context);

    roc_endpoint* source_endpoint;
    CHECK(roc_endpoint_allocate(&source_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(source_endpoint, "rtp://127.0.0.1:10001") == 0);

    roc_endpoint* repair_endpoint;
    CHECK(roc_endpoint_allocate(&repair_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(repair_endpoint, "rs8m://127.0.0.1:10002") == 0);

    roc_endpoint* control_endpoint;
    CHECK(roc_endpoint_allocate(&control_endpoint) == 0);
    CHECK(roc_endpoint_set_uri(control_endpoint, "rtcp://127.0.0.1:10003") == 0);

    // Initialize the Proxy with endpoints
    Proxy proxy_instance(
        source_endpoint, repair_endpoint, control_endpoint, 
        10, 10, 10  // Packet counts for source, repair, and control
    );

    // Verify that endpoints are set correctly
    CHECK(proxy_instance.source_endpoint() == source_endpoint);
    CHECK(proxy_instance.repair_endpoint() == repair_endpoint);
    CHECK(proxy_instance.control_endpoint() == control_endpoint);

    // Simulate communication through Proxy
    // For example, create a dummy packet, send it, and check the result
    packet::PacketPtr dummy_packet = packet::Packet::make();
    dummy_packet->udp()->dst_addr = control_endpoint->address; // Set destination to control
    CHECK(proxy_instance.write(dummy_packet) == status::StatusOK);

    // Clean up
    CHECK(roc_endpoint_deallocate(source_endpoint) == 0);
    CHECK(roc_endpoint_deallocate(repair_endpoint) == 0);
    CHECK(roc_endpoint_deallocate(control_endpoint) == 0);
    CHECK(roc_context_close(context) == 0);
}



} // namespace api
} // namespace roc