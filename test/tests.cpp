#include "server.hpp"

std::string home_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    std::string json_string = R"({"name": "Alice"})";
    return json_string;
}

std::string api_handler(const std::string& path, const std::unordered_map<std::string, std::string>& params) {
    std::unordered_map<std::string, std::string> mutable_params = {
        {"name", "Alice"}, {"age", "34"}, {"id", params.at("id")}};
    std::string json_output = map_to_json(mutable_params);
    return json_output;
}

int main() {
    http::server::server s("127.0.0.1", "8080");
    s.register_handler("GET", "/", home_handler);
    s.register_handler("GET", "/api/v1/users/:id", api_handler);
    s.start();
    return 1;
}