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

}  // namespace http

#endif  // ROUTER_HPP