#include "router.h"

#include "utils.h"

namespace http {

    int Router::RegisterHandler(const std::string& method, const std::string& path, request_handler handler) {
        auto parts = SplitPath(path);
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

    bool Router::Match(http::Request* req, request_handler* out_handler) const {
        std::string path = req->path();
        std::string method = req->method();
        std::string path_without_query;
        std::string query_string;

        // Split path and query string
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path_without_query = path.substr(0, query_pos);
            query_string = path.substr(query_pos + 1);
        } else {
            path_without_query = path;
        }

        auto parts = SplitPath(path_without_query);
        const Node* node = &root;

        for (const auto& part : parts) {
            auto it = node->static_children.find(part);
            if (it != node->static_children.end()) {
                node = it->second.get();
            } else if (node->param_child) {
                req->set_param(node->param_child->param_name, part);
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

        ParseQueryString(query_string, req);

        return true;
    }

    std::vector<std::string> Router::SplitPath(const std::string& path) {
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

    void Router::ParseQueryString(const std::string& query_string, http::Request* req) const {
        if (query_string.empty()) {
            return;
        }

        size_t start = 0;
        while (start < query_string.length()) {
            size_t ampersand_pos = query_string.find('&', start);
            if (ampersand_pos == std::string::npos) {
                ampersand_pos = query_string.length();
            }

            size_t equals_pos = query_string.find('=', start);
            std::string key, value;

            if (equals_pos != std::string::npos && equals_pos < ampersand_pos) {
                key = query_string.substr(start, equals_pos - start);
                value = query_string.substr(equals_pos + 1, ampersand_pos - equals_pos - 1);
            } else {
                key = query_string.substr(start, ampersand_pos - start);
                value = "";
            }

            // URL decode and store
            key = UrlDecode(key);
            value = UrlDecode(value);
            req->set_query(key, value);

            start = ampersand_pos + 1;
        }
    }

}  // namespace http
