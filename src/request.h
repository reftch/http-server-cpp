#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

#include "response.h"

namespace http {

    class Request {
       public:
        Request() = default;
        explicit Request(const std::string& raw_request);

        const std::string& method() const { return method_; }
        const std::string& path() const { return path_; }
        const std::string& version() const { return version_; }
        const std::unordered_map<std::string, std::string>& headers() const { return headers_; }

        // const std::string& mime_type() const { return mime_type_; }
        const std::unordered_map<std::string, std::string>& params() const { return params_; }
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        void set_method(const std::string& method) { method_ = method; }
        void set_path(const std::string& path) { path_ = path; }
        void set_version(const std::string& version) { version_ = version; }
        // void set_mime_type(const std::string& mime_type) { mime_type_ = mime_type; }

        void set_param(const std::string& key, const std::string& value) { params_[key] = value; }

        void set_query(const std::string& key, const std::string& value) { query_[key] = value; }
        // std::string get_mime_type(const std::string& path);
        bool is_keep_alive();
        ContentType mime_type() const { return mime_type_; }

        ContentType GetMimeType(const std::string& path);

       private:
        std::string method_;
        std::string path_;
        std::string version_;
        ContentType mime_type_;

        std::unordered_map<std::string, std::string> headers_;
        std::unordered_map<std::string, std::string> params_;
        std::unordered_map<std::string, std::string> query_;

        void ParseRequestLine(const std::string& line);
        void ParseHeaders(std::istream& ss);
    };

}  // namespace http

#endif  // REQUEST_HPP