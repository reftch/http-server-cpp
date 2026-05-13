#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "request.hpp"

namespace http {

    /**
     * @brief Type alias for HTTP response body
     *
     * Represents the string content that will be returned as the response body
     * to an HTTP request.
     */
    using response_body = std::string;

    /**
     * @brief Type alias for HTTP request handler function
     *
     * A function that handles HTTP requests and returns a response body.
     *
     * @param path The requested URL path
     * @param params URL parameters extracted from the path
     * @param query Query parameters extracted from the path
     * @return response_body The response content to send back to client
     */
    using request_handler = std::function<response_body(const http::request::context& ctx)>;

    /**
     * @brief HTTP router class for mapping URL paths to request handlers
     *
     * This class implements a trie-based routing system that efficiently
     * matches HTTP requests to their corresponding handlers based on method
     * and path. It supports both static paths and parameterized paths.
     */
    class router {
       private:
        /**
         * @brief Internal node structure for the routing trie
         *
         * Each node represents a segment of a URL path and contains:
         * - Static children for literal path segments
         * - A parameter child for dynamic path segments (e.g., :id)
         * - Handlers for specific HTTP methods
         */
        struct Node {
            std::unordered_map<std::string, std::unique_ptr<Node>> static_children;
            std::unique_ptr<Node> param_child;
            std::string param_name;
            std::unordered_map<std::string, request_handler> handlers;
        };

        Node root;

        /**
         * @brief Splits a URL path into its component segments
         *
         * @param path The URL path to split
         * @return std::vector<std::string> Vector of path segments
         */
        static std::vector<std::string> split_path(const std::string& path);

       public:
        /**
         * @brief Registers a request handler for a specific method and path
         *
         * @param method The HTTP method (GET, POST, PUT, DELETE, etc.)
         * @param path The URL path pattern to register
         * @param handler The handler function to call when matched
         * @return int Returns 0 on success, non-zero on failure
         */
        int register_handler(const std::string& method, const std::string& path, request_handler handler);

        /**
         * @brief Matches an HTTP request to a registered handler
         *
         * This function traverses the routing trie to find a matching handler for the
         * given HTTP request. It extracts path parameters from parameterized segments
         * and stores them in the context object's params member.
         *
         * @param ctx Pointer to the request context containing method, path, params, and query
         * @param out_handler Pointer to store the matched handler function (output parameter)
         * @return bool True if a matching handler was found, false otherwise
         */
        bool match(http::request::context* ctx, request_handler* out_handler) const;
    };

}  // namespace http

#endif  // ROUTER_HPP