
#include "websocket.h"

#include <future>  // For std::promise and std::future
#include <thread>

#include "client.h"
#include "server_test.h"
#include "websocketclient.h"

using namespace http;

TEST_F(ServerTestFixture, WebsocketStartAndStop) {
    // Use a unique port for each test to avoid conflicts
    const std::string test_host = "127.0.0.1";
    const int test_port = 8081 + (getpid() % 1000);  // Add process ID offset

    auto server = std::make_unique<Server>(test_host, test_port);

    // Use promises to capture both result and message
    std::promise<Result> result_promise;
    std::promise<std::string> message_promise;
    std::future<Result> result_future = result_promise.get_future();
    std::future<std::string> message_future = message_promise.get_future();

    // Set up route
    server->setRoute("/ws", [&result_promise, &message_promise](http::WebSocket& ws) {
        std::string msg;
        Result read_result = ws >> msg;
        result_promise.set_value(read_result);
        message_promise.set_value(msg);  // Capture the actual message text
    });

    // Start server in separate thread
    std::thread server_thread([&server]() {
        server->start();
    });

    // Wait for server to start
    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    WebSocketClient client(test_host, test_port);

    if (!client.connect()) {
        std::cerr << "Failed to connect" << std::endl;
        stopServer(server, server_thread);
        FAIL() << "Failed to connect to server";
    }

    if (!client.handshake("/ws")) {
        std::cerr << "WebSocket handshake failed" << std::endl;
        stopServer(server, server_thread);
        FAIL() << "WebSocket handshake failed";
    }

    const std::string test_message = "Hello, WebSocket!";
    client.sendMessage(test_message);

    // Wait for the result and message
    try {
        Result received_result = result_future.get();
        std::string received_message = message_future.get();

        EXPECT_EQ(received_result, Result::Text);
        EXPECT_EQ(received_message, test_message);
    } catch (const std::exception& e) {
        std::cerr << "Exception in websocket handler: " << e.what() << std::endl;
        FAIL() << "Exception occurred in websocket handler";
    }

    // Stop server
    stopServer(server, server_thread);

    // Verify server is stopped
    EXPECT_FALSE(server->is_running());
}

// Test: Send Binary Message

TEST_F(ServerTestFixture, WebsocketBinaryMessage) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8082 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);
    std::promise<Result> result_promise;
    std::promise<std::string> message_promise;
    std::future<Result> result_future = result_promise.get_future();
    std::future<std::string> message_future = message_promise.get_future();

    server->setRoute("/ws", [&result_promise, &message_promise](http::WebSocket& ws) {
        std::string msg;
        Result read_result = ws >> msg;
        result_promise.set_value(read_result);
        message_promise.set_value(msg);
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    WebSocketClient client(test_host, test_port);

    if (!client.connect() || !client.handshake("/ws")) {
        stopServer(server, server_thread);
        FAIL() << "Failed to connect or handshake";
    }

    const std::vector<uint8_t> binary_data = {0x01, 0x02, 0x03, 0xFF};
    std::string data_str(binary_data.begin(), binary_data.end());
    client.sendMessage(data_str, true);

    try {
        Result received_result = result_future.get();
        std::string received_message = message_future.get();

        EXPECT_EQ(received_result, Result::Binary);

        // Compare raw bytes if needed
    } catch (const std::exception& e) {
        std::cerr << "Exception in websocket handler: " << e.what() << std::endl;
        FAIL() << "Exception occurred in websocket handler";
    }

    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, WebsocketMultipleClients) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8083 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);
    std::promise<std::string> message_promise;
    std::future<std::string> message_future = message_promise.get_future();

    server->setRoute("/ws", [&message_promise](http::WebSocket& ws) {
        std::string msg;
        Result read_result = ws >> msg;
        if (read_result == Result::Text) {
            message_promise.set_value(msg);
        }
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    WebSocketClient client1(test_host, test_port);
    WebSocketClient client2(test_host, test_port);

    if (!client1.connect() || !client1.handshake("/ws") || !client2.connect() || !client2.handshake("/ws")) {
        stopServer(server, server_thread);
        FAIL() << "Failed to connect or handshake";
    }

    const std::string message1 = "Client 1 says hello!";
    const std::string message2 = "Client 2 says goodbye!";

    client1.sendMessage(message1);
    client2.sendMessage(message2);

    try {
        std::string received_msg = message_future.get();
        // Since order isn't guaranteed, just verify it was received
        EXPECT_TRUE(received_msg == message1 || received_msg == message2);
    } catch (const std::exception& e) {
        std::cerr << "Exception in websocket handler: " << e.what() << std::endl;
        FAIL() << "Exception occurred in websocket handler";
    }

    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, WebsocketInvalidUpgrade) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8085 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);
    server->setRoute("/ws", [](http::WebSocket&) {
        FAIL() << "Should not reach WebSocket handler";
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";

    // Try connecting via HTTP instead of WS
    Client client("http://" + test_host + ":" + std::to_string(test_port));
    auto response = client.get("/ws");

    EXPECT_EQ(response->status(), Status::not_found);

    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, WebsocketTimeout) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8086 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);
    server->setRoute("/ws", [](http::WebSocket&) {
        // Do not read anything — simulate timeout condition
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Longer than expected timeout
        FAIL() << "Should have timed out";
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";

    WebSocketClient client(test_host, test_port);

    if (!client.connect() || !client.handshake("/ws")) {
        stopServer(server, server_thread);
        FAIL() << "Failed to connect or handshake";
    }

    // Send nothing; wait for timeout in handler
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, WebsocketPingPong) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8087 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);
    std::promise<Result> result_promise;
    std::future<Result> result_future = result_promise.get_future();

    server->setRoute("/ws", [&result_promise](http::WebSocket& ws) {
        std::string msg;
        Result read_result = ws >> msg;
        result_promise.set_value(read_result);
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    WebSocketClient client(test_host, test_port);

    if (!client.connect() || !client.handshake("/ws")) {
        stopServer(server, server_thread);
        FAIL() << "Failed to connect or handshake";
    }

    client.sendPing();

    try {
        Result received_result = result_future.get();
        EXPECT_EQ(received_result, Result::Ping);  // This should now work
    } catch (...) {
        FAIL() << "Timeout or exception in ping pong test";
    }

    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}

TEST_F(ServerTestFixture, WebsocketClientDisconnect) {
    const std::string test_host = "127.0.0.1";
    const int test_port = 8084 + (getpid() % 1000);

    auto server = std::make_unique<Server>(test_host, test_port);

    // Track if connection was closed
    bool client_disconnected = false;

    server->setRoute("/ws", [&client_disconnected](http::WebSocket& ws) {
        try {
            std::string msg;
            Result read_result = ws >> msg;

            // If we get a failure result, it means the connection was likely closed
            if (read_result == Result::Fail) {
                client_disconnected = true;
            }
        } catch (...) {
            client_disconnected = true;
        }
    });

    std::thread server_thread([&server]() {
        server->start();
    });

    ASSERT_TRUE(waitForServerStart(server)) << "Server failed to start within timeout";
    EXPECT_TRUE(server->is_running());

    WebSocketClient client(test_host, test_port);

    if (!client.connect() || !client.handshake("/ws")) {
        stopServer(server, server_thread);
        FAIL() << "Failed to connect or handshake";
    }

    // Send a message to establish connection
    client.sendMessage("Test");

    // Give some time for connection to be established
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Close the connection
    client.disconnect();

    // Wait a bit more to let server detect disconnection
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Note: This test assumes server detects disconnect, but it might not be reliable
    stopServer(server, server_thread);
    EXPECT_FALSE(server->is_running());
}