#include <mach/mach.h>
#include <mach/mach_host.h>

#include <charconv>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "response.h"
#include "server.h"
// #include "websocket.h"

// #define HTTP_OPENSSL_SUPPORT
// #include "response.h"
// #include "sslserver.h"

using CpuTicks = std::array<uint64_t, 4>;

CpuTicks readCpuTicks() {
    processor_info_array_t info;
    mach_msg_type_number_t count;
    natural_t cpuCount;

    kern_return_t result = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &info, &count);

    if (result != KERN_SUCCESS) return {0, 0, 0, 0};

    uint64_t user = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t nice = 0;

    for (unsigned i = 0; i < cpuCount; i++) {
        user += info[CPU_STATE_MAX * i + CPU_STATE_USER];
        system += info[CPU_STATE_MAX * i + CPU_STATE_SYSTEM];
        idle += info[CPU_STATE_MAX * i + CPU_STATE_IDLE];
        nice += info[CPU_STATE_MAX * i + CPU_STATE_NICE];
    }

    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(info), count * sizeof(integer_t));

    return {user, system, idle, nice};
}

int getCpuUsage() {
    auto before = readCpuTicks();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto after = readCpuTicks();

    uint64_t totalBefore = before[0] + before[1] + before[2] + before[3];
    uint64_t totalAfter = after[0] + after[1] + after[2] + after[3];

    uint64_t total = totalAfter - totalBefore;
    uint64_t idle = after[2] - before[2];

    if (total == 0) return 0;

    return static_cast<int>(100.0 * (1.0 - static_cast<double>(idle) / total));
}

int main() {
    static auto& log = http::Logger::getInstance();
    // log.setLevel(http::Level::DEBUG);

    // http::Server s("0.0.0.0", 8083);
    // http::SSLServer s("localhost", 8443, "cert.pem", "key.pem");
    http::Server s;

    s.setDefaultHeaders({
        {"Connection", "keep-alive"},
    });

    s.setRoute<http::HttpMethod::GET>("/", [](const http::Request&, http::Response& res) {
        res << http::ContentType::HTML << "dashboard.html";
    });

    s.setRoute<http::HttpMethod::GET>("/api/metrics/cpu", [](const http::Request&, http::Response& res) {
        res << http::ContentType::SSE << "event: connected\n" << "data: connected\n\n";
        res.sendChunk();

        auto res_ptr = std::make_shared<http::Response>(std::move(res));
        std::thread([res_ptr]() {
            auto result = true;
            while (result) {
                auto usage = getCpuUsage();
                *res_ptr << "event: cpu\n" << "data: " << usage << "\n\n";
                result = res_ptr->sendChunk();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }).detach();
    });

    s.setRoute<http::HttpMethod::GET>("/api/v1/inc/:v", [&](const http::Request& req, http::Response& res) {
        auto it = req.params().find("v");
        if (it == req.params().end()) {
            res << http::ContentType::JSON << http::Status::bad_request << R"({"error":"missing parameter 'v'"})";
            return;
        }

        int val = 0;
        if (!std::from_chars(it->second.data(), it->second.data() + it->second.size(), val).ptr) {
            res << http::ContentType::JSON << http::Status::bad_request << R"({"error":"invalid integer"})";
            return;
        }

        res << http::ContentType::JSON << "{\"value\":\"" + std::to_string(val + 1) + "\"}";
    });

    // Post request for CORS
    s.setPostRoute([&](const http::Request&, http::Response& res) {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    s.start();

    return 0;
}
