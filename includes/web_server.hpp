#pragma once

#include <logger.hpp>
#include <thread>
#include <iostream>

#include <http-server/includes/http_server.hpp>
#include <http-server/includes/http_request.hpp>
#include <http-server/includes/http_response.hpp>

#include <web_types.hpp>
#include <web_methods.hpp>
#include <web_request.hpp>
#include <web_response.hpp>
#include <web_router.hpp>
#include <web_exceptions.hpp>
#include <web_utilities.hpp>
#include <thread_pool.hpp>

#define HEADER_RECEIVED_PARAMS std::shared_ptr<hamza_socket::connection> conn,         \
                               const std::multimap<std::string, std::string> &headers, \
                               const std::string &method,                              \
                               const std::string &uri,                                 \
                               const std::string &version,                             \
                               const std::string &body
namespace hamza_web
{
    /**
     * @brief High-level web server template for handling HTTP requests with routing.
     *
     * This class provides a complete web server implementation built on top of the
     * lower-level HTTP server. It manages request routing, static file serving,
     * middleware execution, and worker thread pools for concurrent request handling.
     *
     * Key features:
     * - Multi-threaded request processing with worker pool
     * - Static file serving with MIME type detection
     * - Router registration for dynamic content
     * - Middleware support for cross-cutting concerns
     * - Exception handling with proper HTTP status codes
     *
     * @tparam T Request type (must derive from web_request)
     * @tparam G Response type (must derive from web_response)
     */
    template <typename T = web_request, typename G = web_response>
    class web_server : public hamza_http::http_server
    {
    protected:
        /// Server port number
        uint16_t port;
        /// Server host/IP address
        std::string host;

        /// Thread pool for handling requests concurrently
        thread_pool worker_pool;

        /// Directories to serve static files from
        std::vector<std::string> static_directories;
        /// Registered routers for handling dynamic requests
        std::vector<std::shared_ptr<web_router<T, G>>> routers;

        /// Callback executed when server starts listening
        web_listen_callback_t listen_callback = [this]() -> void
        {
            std::cout << "Server is listening at " << this->host << ":" << this->port << std::endl;
        };

        /// Callback for handling server errors
        std::function<void(const std::exception &)> error_callback = [](const std::exception &e) -> void
        {
            std::string what = e.what();
            logger::error("[Socket Exception]: " + what);
        };

        std::function<void(HEADER_RECEIVED_PARAMS)> headers_callback = nullptr;

        /// Handler for unmatched routes (404 responses)
        web_request_handler_t<T, G> handle_unmatched_route = []([[maybe_unused]] std::shared_ptr<T> req, std::shared_ptr<G> res) -> exit_code
        {
            res->set_status(404, "Not Found");
            res->send_text("404 Not Found");
            return exit_code::EXIT;
        };

        web_unhandled_exception_callback_t<T, G> unhandled_exception_callback = nullptr;

    public:
        /**
         * @brief Construct a web server with specified port and host.
         * @param port Port number to listen on
         * @param host Host address (default: "0.0.0.0" for all interfaces)
         */
        explicit web_server(uint16_t port, const std::string &host = "0.0.0.0")
            : port(port), host(host), hamza_http::http_server(port, host), worker_pool(std::thread::hardware_concurrency())
        {
            static_assert(std::is_base_of<web_request, T>::value, "T must derive from web_request");
            static_assert(std::is_base_of<web_response, G>::value, "G must derive from web_response");
        }

        // Disable copy and move for resource safety
        web_server(const web_server &) = delete;
        web_server &operator=(const web_server &) = delete;
        web_server(web_server &&) = delete;
        web_server &operator=(web_server &&) = delete;

        /**
         * @brief Register a router for handling dynamic requests.
         * @param router Shared pointer to the router to register
         */
        virtual void register_router(std::shared_ptr<web_router<T, G>> router)
        {
            this->routers.push_back(router);
        }

        /**
         * @brief Register a directory for serving static files.
         * @param directory Path to the static files directory
         */
        virtual void register_static(const std::string &directory)
        {
            static_directories.push_back(directory);
        }

        /**
         * @brief Set custom handler for unmatched routes.
         * @param handler Function to handle 404 cases
         */
        virtual void register_unmatched_route_handler(const web_request_handler_t<T, G> &handler)
        {
            handle_unmatched_route = handler;
        }
        /**
         * @brief Register a callback for when headers are received.
         * @note default is no action
         * @param callback
         */
        virtual void register_headers_received_callback(const std::function<void(HEADER_RECEIVED_PARAMS)> &callback)
        {
            headers_callback = callback;
        }

        /**
         * @brief Register a callback for unhandled exceptions.
         * @note if not set, the default server behavior is used
         * @param callback
         */
        virtual void register_unhandled_exception_callback(web_unhandled_exception_callback_t<T, G> callback)
        {
            unhandled_exception_callback = callback;
        }

        /**
         * @brief Start the server and begin listening for requests.
         * @param listen_callback Optional callback for listen success
         * @param error_callback Optional callback for errors
         */
        virtual void listen(web_listen_callback_t listen_callback = nullptr, web_error_callback_t error_callback = nullptr)
        {
            if (listen_callback)
            {
                this->listen_callback = listen_callback;
            }
            if (error_callback)
            {
                this->error_callback = error_callback;
            }
            hamza_http::http_server::listen();
        }

        /**
         * @brief Stop the server and shutdown worker threads.
         */
        virtual void stop()
        {
            hamza_http::http_server::stop_server();
            worker_pool.stop_workers();
        }

    protected:
        /**
         * @brief Serve static files from registered directories.
         * @param req Request object containing URI
         * @param res Response object for sending file content
         */
        virtual void serve_static(std::shared_ptr<T> req, std::shared_ptr<G> res)
        {
            try
            {
                std::string uri = req->get_uri();
                std::string sanitized_path = sanitize_path(uri);
                std::string file_path;

                /// If the file found in the registered static directories
                for (const auto &dir : static_directories)
                {
                    file_path = dir + sanitized_path;
                    if (std::ifstream(file_path))
                    {
                        break;
                    }
                }
                /// No file, bad, return 404
                if (file_path.empty() || !std::ifstream(file_path))
                {
                    res->set_status(404, "Not Found");
                    res->send_text("404 Not Found");
                    return;
                }

                std::ifstream file(file_path);
                std::stringstream buffer;
                buffer << file.rdbuf();
                res->set_body(buffer.str());

                /// send the file to the browser
                res->set_content_type(get_mime_type_from_extension(get_file_extension_from_uri(uri)));
                res->set_status(200, "OK");
                res->send();
            }
            catch (const std::exception &e)
            {
                logger::error("Error serving static file: " + std::string(e.what()));
                web_exception exp(
                    "Error serving static file",
                    "INTERNAL_ERROR",
                    "serve_static",
                    500,
                    "Internal Server Error");
                on_unhandled_exception(req, res, exp);
            }
        }
        /**
         * @brief Main request handler executing routing pipeline.
         * @note This function processes incoming requests by first attempting to serve static files,
         *       and if that fails, it falls back to the registered routers for dynamic handling.
         * @note The function calls res->send() to send the response, then calls res->end() to end the response.
         * @note if No matching route is found, a 404 response is sent by default, user may add their own handlers
         * @param req Request object
         * @param res Response object
         *
         * Processes requests through static file serving and router matching.
         */
        virtual void request_handler(std::shared_ptr<T> req, std::shared_ptr<G> res)
        {
            try
            {
                bool handled = false;              // check if the route got matched with any of the registered routers
                if (is_uri_static(req->get_uri())) // if static, serve the static file
                {

                    serve_static(req, res);
                    handled = true;
                }
                else
                {
                    // check the routers if they can handle the request
                    for (const auto &router : routers)
                    {
                        if (router->handle_request(req, res))
                        {
                            handled = true;
                            break;
                        }
                    }
                }

                if (!handled) // not handled yet, fallback to 404, user may add custom handlers
                    handle_unmatched_route(req, res);

                res->send();
                res->end();
            }
            catch (const std::exception &e)
            {
                logger::error("Error in request handler thread: " + std::string(e.what()));

                web_exception exp(
                    "Error in request handler thread",
                    "INTERNAL_ERROR",
                    "request_handler",
                    500,
                    "Internal Server Error");

                on_unhandled_exception(req, res, exp);
            }

            res->send();
            res->end();
        };

        /**
         * @brief HTTP server callback for incoming requests.
         * @param request Low-level HTTP request object
         * @param response Low-level HTTP response object
         *
         * Converts HTTP objects to web objects and dispatches to worker threads.
         *
         * @note Intended to just pass the HTTP request, response to another thread to handle it
         */
        virtual void on_request_received(hamza_http::http_request &request, hamza_http::http_response &response) override
        {
            auto req = std::make_shared<T>(std::move(request));
            auto res = std::make_shared<G>(std::move(response));

            // If the pointers somehow was not created
            if (!res || !req)
            {
                logger::error("Failed to create request/response objects");
                return;
            }

            // If an invalid HTTP method is received
            if (unknown_method(req->get_method()))
            {
                logger::error("Unknown HTTP method: " + req->get_method());

                // Send back 405 Method Not Allowed
                res->set_status(405, "Method Not Allowed");
                res->send_text("405 Method Not Allowed");
                res->end();
                return;
            }
            try
            {
                // Enqueue the request handler for processing
                worker_pool.enqueue([this, req, res]()
                                    { request_handler(req, res); });
            }
            catch (web_exception &e) // Unhandled web_exception
            {

                logger::error("Error in request handler thread: " + std::string(e.what()));

                on_unhandled_exception(req, res, e);

                res->send();
                res->end();
            }
            catch (const std::exception &e) // unexpected exception
            {
                logger::error("Error in request handler thread: " + std::string(e.what()));

                web_exception exp(
                    "Error in request handler thread",
                    "INTERNAL_ERROR",
                    "request_handler",
                    500,
                    "Internal Server Error");

                on_unhandled_exception(req, res, exp);
                res->send();
                res->end();
            }
        };

        /// HTTP server callback for successful listen
        virtual void on_listen_success() override
        {
            this->listen_callback();
        }

        /// @brief HTTP server callback for exceptions
        /// @param e The exception that occurred
        virtual void on_exception_occurred(const std::exception &e) override
        {
            this->error_callback(e);
        }

        /// @brief HTTP server callback for header processing, you may want to log or modify headers, or even close the connection
        /// @note use close_connection(conn) to close the connection if needed
        /// @param conn The connection object
        /// @param headers The headers received
        /// @param method The HTTP method
        /// @param uri The request URI
        /// @param version The HTTP version
        /// @param body The request body
        virtual void on_headers_received(HEADER_RECEIVED_PARAMS) override
        {
            if (headers_callback)
                headers_callback(conn, headers, method, uri, version, body);
        }

        /**
         * @brief Handle unhandled web exceptions
         * @note This function is called when an unhandled exception occurs.
         * @note You can provide a callback via set_unhandled_exception_callback., if no callback set, it will log the error and send a 500 response.
         * @param req The HTTP request
         * @param res The HTTP response
         * @param e The exception that occurred
         */
        virtual void on_unhandled_exception(std::shared_ptr<T> req, std::shared_ptr<G> res, web_exception &e)
        {
            if (unhandled_exception_callback)
            {
                unhandled_exception_callback(req, res, e);
                return;
            }
            res->set_status(e.get_status_code(), e.get_status_message());
            res->send_text("Internal Server Error");
            logger::error("Unhandled Web exception: " + std::string(e.what()));
            res->end();
        }
    };

}