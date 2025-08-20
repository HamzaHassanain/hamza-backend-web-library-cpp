#pragma once
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

namespace hamza_web
{
    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_server
    {
    private:
        using ret = std::function<void(std::shared_ptr<hamza::socket_exception>)>;
        using param = std::function<void(std::shared_ptr<hamza_web::web_general_exception>)>;

        // ret custom_wrap(param &callback)
        // {
        //     return [callback = std::move(callback)](std::shared_ptr<hamza::socket_exception> e) -> void
        //     {
        //         if (auto web_exc = std::dynamic_pointer_cast<hamza_web::web_general_exception>(e))
        //         {
        //             callback(web_exc);
        //         }
        //         else
        //         {
        //             callback(std::make_shared<hamza_web::web_general_exception>(e->what()));
        //         }
        //     };
        // }

    public:
        explicit web_server(const std::string &host, uint16_t port)
            : host(host), port(port), server(host, port, 0, 500000)
        {
            static_assert(std::is_base_of<web_request, RequestType>::value, "RequestType must derive from web_request");
            static_assert(std::is_base_of<web_response, ResponseType>::value, "ResponseType must derive from web_response");
            set_default_callbacks();
        }

        // Disable copy and move
        web_server(const web_server &) = delete;
        web_server &operator=(const web_server &) = delete;
        web_server(web_server &&) = delete;
        web_server &operator=(web_server &&) = delete;

        virtual void register_router(std::shared_ptr<web_router<RequestType, ResponseType>> router)
        {
            this->routers.push_back(router);
        }

        virtual void register_static(const std::string &directory)
        {
            static_directories.push_back(directory);
        }
        virtual void register_unmatched_route_handler(const web_request_handler_t<RequestType, ResponseType> &handler)
        {
            handle_unmatched_route = handler;
        }
        virtual void listen(web_listen_success_callback_t callback = nullptr,
                            web_error_callback_t error_callback = nullptr)
        {
            if (callback)
                server.set_listen_success_callback(callback);
            // if (error_callback)
            //     server.set_error_callback(custom_wrap(error_callback));

            server.listen();
        }

    protected:
        std::string host;
        uint16_t port;
        std::vector<std::string> static_directories;
        hamza_http::http_server server;
        std::vector<std::shared_ptr<web_router<RequestType, ResponseType>>> routers;

        virtual void set_default_callbacks()
        {
            server.set_request_callback(request_callback);
            server.set_listen_success_callback(listen_success_callback);
            server.set_error_callback((error_callback));
        }

        virtual void server_static(std::shared_ptr<RequestType> req, std::shared_ptr<ResponseType> res)
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
                std::cerr << "Error serving static file: " << e.what() << std::endl;
                res->set_status(500, "Internal Server Error");
                res->send_text("500 Internal Server Error: " + std::string(e.what()));
            }
        }
        void request_handler([[maybe_unused]] std::shared_ptr<RequestType> req, std::shared_ptr<ResponseType> res)
        {
            bool handled = false;
            if (is_uri_static(req->get_uri()))
            {

                server_static(req, res);
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
        };
        web_request_handler_t<RequestType, ResponseType> handle_unmatched_route = []([[maybe_unused]] std::shared_ptr<RequestType> req, std::shared_ptr<ResponseType> res) -> exit_code
        {
            res->set_status(404, "Not Found");
            res->send_text("404 Not Found");
            return exit_code::EXIT;
        };

        http_request_callback_t request_callback = [this](hamza_http::http_request &request, hamza_http::http_response &response) -> void
        {
            RequestType web_req(std::move(request));
            ResponseType web_res(std::move(response));

            auto web_res_ptr = std::make_shared<ResponseType>(std::move(web_res));
            auto web_req_ptr = std::make_shared<RequestType>(std::move(web_req));

            try
            {
                std::thread(&web_server::request_handler, this, web_req_ptr, web_res_ptr).detach();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error handling request: " << e.what() << std::endl;
                web_res_ptr->set_status(500, "Internal Server Error");
                web_res_ptr->send_text("500 Internal Server Error");
                web_res_ptr->end();
            }
        };

        web_listen_success_callback_t listen_success_callback = [this]() -> void
        {
            std::cout << "Server is listening at " << this->host << ":" << this->port << std::endl;
        };

        std::function<void(std::shared_ptr<hamza::socket_exception>)> error_callback = [](std::shared_ptr<hamza::socket_exception> e) -> void
        {
            if (!e)
            {
                std::cerr << "Unknown error occurred." << std::endl;
                return;
            }
            // std::cerr << "Error occurred: " << e->get_status_code() << ": " << e->get_status_message() << std::endl;
            // std::cerr << "Error occurred: " << e->type() << std::endl;
            std::cerr << "Error occurred: " << e->what() << std::endl;
        };
    };

}