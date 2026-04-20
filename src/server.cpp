#include "server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace http::server
{
    /**
     * Constructor implementation.
     */
    server::server(const std::string &host, const std::string &port)
        : host_(host), port_(port)
    {
        // std::cout << "[Server Setup] Initializing server for host: " << host_
        //           << ", port: " << port_ << std::endl;
    }

    /**
     * Implementation of the start method.
     * In a real application, this method would set up sockets and begin listening.
     */
    void server::start()
    {
        std::cout << "Server started on http:// " << host_ << ":" << port_ << std::endl;

        // Simulate server running in a separate thread (for demonstration)
        // std::cout << "[Server Running] Server is now listening for requests..." << std::endl;

        // In a real application, the main listening loop would happen here.
        // We simulate a short delay.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

} // namespace http::server