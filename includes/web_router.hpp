#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

#include <web_types.hpp>
#include <web_exceptions.hpp>
#include <web_request.hpp>
#include <web_response.hpp>
#include <web_methods.hpp>
namespace hamza_web
{
    template <typename RequestType, typename ResponseType>
    class web_route; // Forward declaration

    template <typename RequestType, typename ResponseType>
    class web_server; // Forward declaration

    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_router
    {
        std::vector<std::shared_ptr<web_route<RequestType, ResponseType>>> routes;
        std::vector<web_request_handler_t<RequestType, ResponseType>> middlewares;

        exit_code middleware_handle_request(std::shared_ptr<RequestType> request, std::shared_ptr<ResponseType> response)
        {
            for (const auto &middleware : middlewares)
            {
                auto result = middleware(request, response);
                if (result == exit_code::EXIT)
                {
                    return exit_code::EXIT;
                }

                else if (result == exit_code::ERROR)
                {
                    return exit_code::ERROR;
                }

                else if (result == exit_code::CONTINUE)
                {
                    continue;
                }
                else
                {
                    throw std::runtime_error("Invalid middleware, return value must of  web_hamza::exit_code\n");
                }
            }

            return exit_code::CONTINUE;
        }

        exit_code route_handle_request(std::shared_ptr<RequestType> request, std::shared_ptr<ResponseType> response)
        {
            for (const auto &route : routes)
            {
                if (route->match(request->get_method(), request->get_path()))
                {
                    auto &handlers = route->handlers;
                    for (const auto &handler : handlers)
                    {
                        auto resp = handler(request, response);
                        if (resp == exit_code::EXIT)
                        {
                            return exit_code::EXIT;
                        }
                        else if (resp == exit_code::ERROR)
                        {
                            return exit_code::ERROR;
                        }
                        else if (resp == exit_code::CONTINUE)
                        {
                            continue;
                        }
                        else
                        {
                            throw std::runtime_error("Invalid route handler, return value must of  web_hamza::exit_code\n");
                        }
                    }
                }
            }

            return exit_code::CONTINUE;
        }

    public:
        friend class web_server<RequestType, ResponseType>;

        web_router()
        {
            static_assert(std::is_base_of<web_request, RequestType>::value, "RequestType must derive from web_request");
            static_assert(std::is_base_of<web_response, ResponseType>::value, "ResponseType must derive from web_response");
        }

        web_router(const web_router &) = delete;
        web_router &operator=(const web_router &) = delete;
        web_router(web_router &&) = default;
        web_router &operator=(web_router &&) = default;

        bool handle_request(std::shared_ptr<RequestType> request, std::shared_ptr<ResponseType> response)
        {
            try
            {
                exit_code middleware_result = middleware_handle_request(request, response);

                if (middleware_result != exit_code::CONTINUE)
                {
                    return true;
                }

                exit_code route_result = route_handle_request(request, response);
                if (route_result != exit_code::CONTINUE)
                {
                    return true;
                }

                return false;
            }
            catch (const web_general_exception &e)
            {
                response->set_status(e.get_status_code(), e.get_status_message());
                response->send_text(e.what());
                return true;
            }
            catch (const std::exception &e)
            {
                response->set_status(500, "Internal Server Error");
                response->send_text("500 Internal Server Error: " + std::string(e.what()));
                return true;
            }
        }
        void register_route(std::shared_ptr<web_route<RequestType, ResponseType>> route)
        {
            if (route->get_path().empty())
            {
                throw std::invalid_argument("Route path cannot be empty");
            }
            routes.push_back(route);
        }

        void register_middleware(const web_request_handler_t<RequestType, ResponseType> &middleware)
        {
            middlewares.push_back(middleware);
        }
    };
}

#include <web_route.hpp> // Include after the template declaration