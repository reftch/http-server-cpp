
#include "websocket.h"

#include "client.h"
#include "server_test.h"

using namespace http;

// TODO: not completed yet
TEST_F(ServerTestFixture, WebsocketStartAndStop) {
    // Use a unique port for each test to avoid conflicts
    const std::string test_host = "127.0.0.1";
    const int test_port = 8081 + (getpid() % 1000);  // Add process ID offset

    auto server = std::make_unique<Server>(test_host, test_port);

    static Result result = Result::Fail;
    // Set up route
    server->setRoute("/ws", [](http::WebSocket& ws) {
        std::string msg;
        result = ws >> msg;
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    // Create client and make request to specific host/port
    // auto cli = Client("http://" + test_host + ":" + std::to_string(test_port));
    // auto res = cli.get("/ws");

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(result, Result::Fail);

    // Stop server
    stopServer(server, server_thread);

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}
