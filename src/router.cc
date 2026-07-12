#include "router.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

namespace http {

    int Router::registerHandler(const std::string& method, const std::string& path, request_handler handler) {
        auto parts = splitPath(path);

        // Use accumulate to traverse/build the tree
        // The "accumulator" is the Node* pointer to the current node
        Node* node =
            std::accumulate(parts.begin(), parts.end(), &root_, [](Node* current, const std::string& part) -> Node* {
                if (part.starts_with(':')) {
                    if (!current->param_child) {
                        current->param_child = std::make_unique<Node>();
                    }
                    current->param_child->param_name = part.substr(1);
                    return current->param_child.get();
                }

                // Handles static children (including empty strings)
                auto& child = current->static_children[part];
                if (!child) {
                    child = std::make_unique<Node>();
                }
                return child.get();
            });

        // Final registration logic
        if (node->handlers.contains(method)) {
            return -1;
        }

        node->handlers[method] = std::move(handler);
        return 0;
    }

    bool Router::match(http::Request* req, request_handler* out_handler) const {
        std::string_view full_path = req->path();

        // Split path and query string using string_view (zero-allocation split)
        auto query_pos = full_path.find('?');
        std::string_view path_without_query =
            full_path.substr(0, query_pos != std::string::npos ? query_pos : full_path.size());

        // Traverse the tree using std::accumulate (The "Fold" alternative)
        auto parts = splitPath(std::string(path_without_query));

        // We accumulate an optional node pointer
        // If at any point a node is missing, we return nullopt and stop progressing
        auto final_node_opt = std::accumulate(
            parts.begin(), parts.end(), std::optional<const Node*>(&root_),
            [&](std::optional<const Node*> current, const std::string& part) -> std::optional<const Node*> {
                if (!current) return std::nullopt;

                auto& node = *current;
                if (auto it = node->static_children.find(part); it != node->static_children.end()) {
                    return it->second.get();
                } else if (node->param_child) {
                    req->setParam(node->param_child->param_name, part);
                    return node->param_child.get();
                }
                return std::nullopt;
            });

        // Check for handler and process results
        if (final_node_opt.has_value()) {
            const auto& node = *final_node_opt;
            if (auto it = node->handlers.find(req->method()); it != node->handlers.end()) {
                // 4. Populate query parameters using for_each (No raw loop)
                std::ranges::for_each(req->query(), [&](const auto& item) {
                    req->setQuery(item.first, item.second);
                });

                *out_handler = it->second;
                return true;
            }
        }

        return false;
    }

    std::vector<std::string> Router::splitPath(const std::string& path) {
        return path | std::views::split('/')  // Split into sub-ranges delimited by '/'
               | std::views::filter([](auto&& r) {
                     return !std::ranges::empty(r);
                 })                                            // Remove empty parts (e.g., "//" or leading "/")
               | std::ranges::to<std::vector<std::string>>();  // Convert sub-ranges to strings and collect into vector
    }

}  // namespace http
