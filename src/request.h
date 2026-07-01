#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <optional>
#include <string>
#include <unordered_map>

namespace http {

    class Request {
       public:
        Request() = default;
        explicit Request(const std::string& raw_request);

        const std::string& method() const { return method_; }
        const std::string& path() const { return path_; }
        const std::string& normalize_path() const { return normalize_path_; }
        const std::string& query_string() const { return query_string_; }
        const std::string& version() const { return version_; }
        const std::string& body() const { return body_; }
        const std::unordered_map<std::string_view, std::string_view>& headers() const { return headers_; }

        const std::unordered_map<std::string, std::string>& params() const { return params_; }
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        void setMethod(const std::string& method) { method_ = method; }
        void setPath(const std::string& path) { path_ = path; }
        void setVersion(const std::string& version) { version_ = version; }
        void setMimeType(const std::string& mime_type) { mime_type_ = mime_type; }

        void setParam(const std::string& key, const std::string& value) { params_[key] = value; }
        void setQuery(const std::string& key, const std::string& value) { query_[key] = value; }

        bool isKeepAlive();

        std::optional<std::string> mimeType() const {
            if (mime_type_.empty()) {
                return std::nullopt;
            }
            return mime_type_;
        }

        std::string getMimeType(const std::string& path);

       private:
        std::string method_;
        std::string path_;
        std::string normalize_path_;
        std::string query_string_;
        std::string version_;
        std::string mime_type_;
        std::string body_;

        std::unordered_map<std::string_view, std::string_view> headers_;
        std::unordered_map<std::string, std::string> params_;
        std::unordered_map<std::string, std::string> query_;

        void parseRequestLine(std::string_view line);
        void parseHeaders(std::string_view raw_request, size_t& pos);
        void parseQueryString(const std::string& query_string);
    };

}  // namespace http

#endif  // REQUEST_HPP