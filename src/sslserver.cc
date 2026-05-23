#include "sslserver.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <iostream>

namespace http {

    bool SSLServer::InitializeSSL() {
        // Initialize OpenSSL
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        // Create SSL context
        const SSL_METHOD* method = TLS_server_method();
        ssl_ctx_ = SSL_CTX_new(method);
        if (!ssl_ctx_) {
            log.Error("Failed to create SSL context");
            return false;
        }

        // Set SSL options
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_SSLv2);
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_SSLv3);
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_COMPRESSION);
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_CIPHER_SERVER_PREFERENCE);
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_SINGLE_DH_USE);
        SSL_CTX_set_options(ssl_ctx_, SSL_OP_SINGLE_ECDH_USE);

        // Set certificate and key
        if (SSL_CTX_use_certificate_file(ssl_ctx_, cert_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
            log.Error("Failed to load certificate file: " + cert_file_);
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file_.c_str(), SSL_FILETYPE_PEM) <= 0) {
            log.Error("Failed to load private key file: " + key_file_);
            ERR_print_errors_fp(stderr);
            return false;
        }

        // Verify that the private key matches the certificate
        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            log.Error("Private key does not match the certificate");
            return false;
        }

        return true;
    }

    void SSLServer::CleanupSSL() {
        if (ssl_ctx_) {
            SSL_CTX_free(ssl_ctx_);
            ssl_ctx_ = nullptr;
        }
        EVP_cleanup();
    }

    SSL* SSLServer::CreateSSLConnection(int client_fd) {
        SSL* ssl = SSL_new(ssl_ctx_);
        if (!ssl) {
            log.Error("Failed to create SSL connection");
            return nullptr;
        }

        SSL_set_fd(ssl, client_fd);

        // Set SSL to non-blocking mode
        SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

        return ssl;
    }

    int SSLServer::Start() {
        // Initialize SSL first
        if (!InitializeSSL()) {
            return 5;  // SSL context initialization error
        }

        // Call parent Start method
        int result = Server::Start();

        // Cleanup SSL context on exit
        CleanupSSL();

        return result;
    }

    void SSLServer::PerformRequest(const int sd, const char* buffer, const ssize_t nread) {
        // Create SSL connection
        SSL* ssl = CreateSSLConnection(sd);
        if (!ssl) {
            close(sd);
            return;
        }

        // Perform SSL handshake
        int ssl_result = SSL_accept(ssl);
        if (ssl_result <= 0) {
            log.Error("SSL handshake failed");
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(sd);
            return;
        }

        // Process request through SSL
        std::string request_data(buffer, nread);
        std::string raw_request(buffer, nread);
        // Parse the request line to find method and path
        http::Request req(raw_request);

        std::string response_body = HandleRoute(req);
        // http::Request ctx;
        SSL_write(ssl, response_body.c_str(), response_body.length());

        // try {
        //     // Parse the request
        //     ctx.Parse(request_data);

        //     // Handle route and get response

        //     // Send response through SSL

        // } catch (const std::exception& e) {
        //     log.Error("Error processing request: " + std::string(e.what()));
        //     // Send error response through SSL
        //     std::string error_response = "<html><body><h1>Internal Server Error</h1></body></html>";
        //     SSL_write(ssl, error_response.c_str(), error_response.length());
        // }

        // Clean up
        SSL_free(ssl);
        close(sd);
    }

    std::string SSLServer::HandleRoute(http::Request& ctx) {
        // Use parent's route handling logic but return the response body
        Response res(ctx.is_keep_alive(), static_directory_);

        // check on static resource
        if (ctx.mime_type().has_value()) {
            auto content = ReadFile(static_directory_ + ctx.path());
            if (content != "") {
                res.SetContentByType(content, ctx.mime_type().value());
            }
        } else {
            // Call handler if it exists
            http::request_handler handler;
            if (router_.Match(&ctx, &handler)) {
                handler(ctx, res);
            } else {
                res.SetContent<ContentType::PLAIN_TEXT, Status::not_found>("Not Found");
            }
        }

        return res.Build();
    }

}  // namespace http