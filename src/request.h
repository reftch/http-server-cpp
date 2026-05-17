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

        const std::unordered_map<std::string, std::string>& params() const { return params_; }
        const std::unordered_map<std::string, std::string>& query() const { return query_; }

        void set_param(const std::string& key, const std::string& value) { params_[key] = value; }
        void set_query(const std::string& key, const std::string& value) { query_[key] = value; }

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