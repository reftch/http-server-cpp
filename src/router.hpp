#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace http {

    using response_body = std::string;
    using request_handler = std::function<response_body(const std::string& path,
                                                        const std::unordered_map<std::string, std::string>& params)>;

    class router {
       private:
        struct Node {
            std::unordered_map<std::string, std::unique_ptr<Node>> static_children;
            std::unique_ptr<Node> param_child;
            std::string param_name;
            std::unordered_map<std::string, request_handler> handlers;
        };

        Node root;

        static std::vector<std::string> split_path(const std::string& path);

       public:
        int register_handler(const std::string& method, const std::string& path, request_handler handler);

        bool match(const std::string& method, const std::string& path, request_handler* out_handler,
                   std::unordered_map<std::string, std::string>* out_params) const;
    };

    // --- inline method definitions ---

    inline int router::register_handler(const std::string& method, const std::string& path, request_handler handler) {
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

    inline bool router::match(const std::string& method, const std::string& path, request_handler* out_handler,
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

    inline std::vector<std::string> router::split_path(const std::string& path) {
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

}  // namespace http

#endif  // ROUTER_HPP