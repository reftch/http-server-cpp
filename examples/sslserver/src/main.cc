#include <signal.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include "sslserver.h"

int main() {
    // static auto& log = http::Logger::getInstance();
    http::SSLServer s("0.0.0.0", 8443, "cert.pem", "key.pem");

    // Register signal handler with capture
    static auto s_ptr = &s;
    signal(SIGINT, [](int) {
        s_ptr->Stop();
    });

    s.SetRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res.SetContent<http::ContentType::PLAIN_TEXT>("Hello, HTTPS!");
    });

    s.Start();

    return 0;
}