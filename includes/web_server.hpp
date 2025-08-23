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

namespace hamza_web
{
    template <typename T = web_request, typename G = web_response>
    class web_server
    {
    protected:
        uint16_t port;
        std::string host;
        hamza_http::http_server server;
        thread_pool worker_pool;

        std::vector<std::string> static_directories;
        std::vector<std::shared_ptr<web_router<T, G>>> routers;

    public:
        explicit web_server(uint16_t port, const std::string &host = "0.0.0.0", size_t thread_pool_size = std::thread::hardware_concurrency(), int milliseconds = 1000)
            : port(port), host(host), server(port, host, milliseconds), worker_pool(thread_pool_size)
        {
            static_assert(std::is_base_of<web_request, T>::value, "T must derive from web_request");
            static_assert(std::is_base_of<web_response, G>::value, "G must derive from web_response");
            set_default_callbacks();
        }

        // Disable copy and move
        web_server(const web_server &) = delete;
        web_server &operator=(const web_server &) = delete;
        web_server(web_server &&) = delete;
        web_server &operator=(web_server &&) = delete;

        virtual void register_router(std::shared_ptr<web_router<T, G>> router)
        {
            this->routers.push_back(router);
        }

        virtual void register_static(const std::string &directory)
        {
            static_directories.push_back(directory);
        }
        virtual void register_unmatched_route_handler(const web_request_handler_t<T, G> &handler)
        {
            handle_unmatched_route = handler;
        }
        virtual void listen(web_listen_success_callback_t callback = nullptr,
                            web_error_callback_t error_callback = nullptr)
        {
            if (callback)
                server.set_listen_success_callback(callback);
            if (error_callback)
                server.set_error_callback(error_callback);

            server.listen();
        }

        virtual void stop()
        {
            server.stop_server();
            worker_pool.stop_workers();
        }

    protected:
        virtual void set_default_callbacks()
        {
            server.set_request_callback(request_callback);
            server.set_listen_success_callback(listen_success_callback);
            server.set_error_callback((error_callback));
        }

        virtual void serve_static(std::shared_ptr<T> req, std::shared_ptr<G> res)
        {
            try
            {
                std::string uri = req->get_uri();
                std::string sanitized_path = sanitize_path(uri);
                std::string file_path;
                for (const auto &dir : static_directories)
                {
                    file_path = dir + sanitized_path;
                    if (std::ifstream(file_path))
                    {
                        break;
                    }
                }

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
                res->set_content_type(get_mime_type_from_extension(get_file_extension_from_uri(uri)));
                res->set_status(200, "OK");
                res->send();
            }
            catch (const std::exception &e)
            {
                Logger::LogError("Error serving static file: " + std::string(e.what()));
                res->set_status(500, "Internal Server Error");
                res->send_text("500 Internal Server Error: " + std::string(e.what()));
            }
        }
        virtual void request_handler(std::shared_ptr<T> req, std::shared_ptr<G> res)
        {
            try
            {

                bool handled = false;
                if (is_uri_static(req->get_uri()))
                {

                    serve_static(req, res);
                    handled = true;
                }
                else
                {
                    for (const auto &router : routers)
                    {
                        if (router->handle_request(req, res))
                        {
                            handled = true;
                            break;
                        }
                    }
                }
                if (!handled)
                    handle_unmatched_route(req, res);

                res->send();
                res->end();
            }
            catch (const std::exception &e)
            {
                Logger::LogError("Error in request handler thread: " + std::string(e.what()));
                res->set_status(500, "Internal Server Error In Thread");
                res->send_text("500 Internal Server Error: " + std::string(e.what()));
            }
            res->end();
        };
        
        
        web_request_handler_t<T, G> handle_unmatched_route = []([[maybe_unused]] std::shared_ptr<T> req, std::shared_ptr<G> res) -> exit_code
        {
            res->set_status(404, "Not Found");
            res->send_text("404 Not Found");
            return exit_code::EXIT;
        };

        http_request_callback_t request_callback = [this](hamza_http::http_request &request, hamza_http::http_response &response) -> void
        {
            auto web_req_ptr = std::make_shared<T>(std::move(request));
            auto web_res_ptr = std::make_shared<G>(std::move(response));
            if (!web_res_ptr || !web_req_ptr)
            {
                Logger::LogError("Failed to create request/response objects");
                return;
            }
            if (unknown_method(web_req_ptr->get_method()))
            {
                Logger::LogError("Unknown HTTP method: " + web_req_ptr->get_method());
                web_res_ptr->set_status(405, "Method Not Allowed");
                web_res_ptr->send_text("405 Method Not Allowed");
                web_res_ptr->end();
                return;
            }
            try
            {
                worker_pool.enqueue([this, web_req_ptr, web_res_ptr]()
                                    { request_handler(web_req_ptr, web_res_ptr); });
            }
            catch (const web_general_exception &e)
            {
                Logger::LogError("Error in request handler thread: " + std::string(e.what()));
                web_res_ptr->set_status(400, "Bad Request");
                web_res_ptr->send_text("400 Bad Request: " + std::string(e.what()));
                web_res_ptr->end();
            }
            catch (const std::exception &e)
            {
                Logger::LogError("Error in request handler thread: " + std::string(e.what()));
                web_res_ptr->set_status(500, "Internal Server Error");
                web_res_ptr->send_text("500 Internal Server Error");
                web_res_ptr->end();
            }
        };

        web_listen_success_callback_t listen_success_callback = [this]() -> void
        {
            std::cout << "Server is listening at " << this->host << ":" << this->port << std::endl;
        };

        std::function<void(const std::exception &)> error_callback = [](const std::exception &e) -> void
        {
            std::string what = e.what();

            Logger::LogError("[Socket Exception]: " + what);
        };
    };

}