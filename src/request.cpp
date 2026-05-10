#include "request.hpp"

#include <string>

namespace http {

    namespace request {

        /**
         * @brief Parses the first line of a raw HTTP request string to extract method, path, and version.
         *
         * This function assumes the request line is in the format: METHOD PATH VERSION (e.g., "GET /file HTTP/1.1").
         *
         * @param raw_request The raw string containing the HTTP request.
         * @return http_request A structure containing the extracted method, path, and version.
         */
        http_request parse(const std::string& raw_request) {
            http_request req;

            // Use stringstream to easily read the input string line by line
            std::stringstream ss(raw_request);
            std::string line;

            // Attempt to read the first line of the request
            if (std::getline(ss, line)) {
                // Use another stringstream to parse the tokens on the extracted line
                std::stringstream line_ss(line);
                std::string method, path, version;

                // Attempt to extract the three tokens (method, path, version) separated by spaces
                if (line_ss >> method >> path >> version) {
                    req.method = method;
                    req.path = path;
                    req.version = version;
                    req.mime_type = get_mime_type(path);
                }
                // Note: If the format is incorrect, req remains default-initialized.
            }

            return req;
        }

        std::string get_mime_type(const std::string& path) {
            std::map<std::string, std::string> mimeTypes = {{"html", "text/html"},
                                                            {"css", "text/css"},
                                                            {"js", "text/javascript"},
                                                            {"jpg", "image/jpeg"},
                                                            {"png", "image/png"}};
            std::string fileExtension = path.substr(path.find_last_of(".") + 1);
            // std::cout<<fileExtension<<std::endl;
            return mimeTypes[fileExtension];
        }
    }  // namespace request

}  // namespace http