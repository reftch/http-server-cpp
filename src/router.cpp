// router.cpp
#include "router.hpp"

/**
 * @brief Registers a request handler for a specific HTTP method and URL path
 *
 * This function builds the routing trie structure by creating nodes for each
 * path segment. It supports both static path segments and parameterized segments
 * (indicated by a leading colon ':').
 *
 * @param method The HTTP method (GET, POST, PUT, DELETE, etc.) to register for
 * @param path The URL path pattern to register (e.g., "/users/:id/profile")
 * @param handler The request handler function to associate with this path
 * @return int Returns 0 on successful registration, -1 if a handler already exists for this method/path combination
 */
int http::router::register_handler(const std::string& method, const std::string& path, request_handler handler) {
    auto parts = split_path(path);
    Node* node = &root;

    for (const auto& part : parts) {
        if (!part.empty() && part[0] == ':') {
            if (!node->param_child) {
                node->param_child = std::make_unique<Node>();
            }
            node->param_child->param_name = part.substr(1);  // strip ':'
            node = node->param_child.get();
        } else {
            auto& child = node->static_children[part];
            if (!child) {
                child = std::make_unique<Node>();
            }
            node = child.get();
        }
    }

    if (node->handlers.count(method) > 0) {
        return -1;
    }
    node->handlers[method] = std::move(handler);
    return 0;
}

/**
 * @brief Matches an HTTP request to a registered handler
 *
 * This function traverses the routing trie to find a matching handler for the
 * given HTTP method and path. It extracts path parameters from parameterized
 * segments and stores them in the output parameter map.
 *
 * @param method The HTTP method of the request (GET, POST, etc.)
 * @param path The URL path of the request
 * @param out_handler Pointer to store the matched handler function (output parameter)
 * @param out_params Pointer to store extracted path parameters (output parameter)
 * @return bool True if a matching handler was found, false otherwise
 */
bool http::router::match(const std::string& method, const std::string& path, request_handler* out_handler,
                         std::unordered_map<std::string, std::string>* out_params) const {
    *out_params = {};
    auto parts = split_path(path);
    const Node* node = &root;

    for (const auto& part : parts) {
        auto it = node->static_children.find(part);
        if (it != node->static_children.end()) {
            node = it->second.get();
        } else if (node->param_child) {
            (*out_params)[node->param_child->param_name] = part;
            node = node->param_child.get();
        } else {
            return false;
        }
    }

    auto it = node->handlers.find(method);
    if (it == node->handlers.end()) {
        return false;
    }

    *out_handler = it->second;
    return true;
}

/**
 * @brief Splits a URL path into its component segments
 *
 * This helper function parses a URL path string and splits it into individual
 * path segments using '/' as the delimiter. Empty segments (from leading/trailing
 * slashes) are ignored.
 *
 * @param path The URL path to split (e.g., "/users/:id/profile")
 * @return std::vector<std::string> Vector of path segments
 */
std::vector<std::string> http::router::split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}